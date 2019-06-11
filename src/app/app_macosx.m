
#include <drive/app.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


/* -------------------------------------------------------------- Platform -- */


@interface macos_app_delegate : NSObject<NSApplicationDelegate> {}
@end

@implementation macos_app_delegate
- (void)applicationDidFinishLaunching:
        (NSNotification *)notification
{
        (void)notification;
        
        [NSApp stop:nil];
    
        NSEvent *evt = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                location:NSMakePoint(0, 0)
                modifierFlags:0
                timestamp:0
                windowNumber:0
                context:nil
                subtype:0
                data1:0
                data2:0];
        
        [NSApp postEvent:evt atStart:YES];
}
@end

@interface macos_window_delegate : NSObject<NSWindowDelegate> {
        @public uint32_t try_close;
}
@end

@implementation macos_window_delegate
- (BOOL)windowShouldClose:
        (NSWindow *)sender
{
        (void)sender;
        
        try_close++;
        return NO;
}
@end

@interface macos_metal_view : NSView {}
@end

@implementation macos_metal_view
- (BOOL)wantsUpdateLayer
{
        return YES;
}

- (CALayer *)makeBackingLayer
{
        Class layer_class = NSClassFromString(@"CAMetalLayer");
        
        return [layer_class layer];
}
@end


/* -------------------------------------------------------------- Lifetime -- */


struct drv_app_ctx {
        macos_window_delegate *window_delegate;
        CAMetalLayer * metal_layer;
        macos_metal_view * metal_view;
};


drv_app_result
drv_app_ctx_create(
        const struct drv_plat_ctx_create_desc *desc,
        struct drv_app_ctx **out)
{
        struct drv_app_ctx *ctx = malloc(sizeof(*ctx));
        
        /* app */
        [[NSAutoreleasePool alloc] init];
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        /* name */
        NSString * app_name = [NSString stringWithUTF8String:desc->title];

        /* menu */
        NSMenu * men_bar = 0;
        men_bar = [[NSMenu alloc] init];
        [men_bar autorelease];
        
        [NSApp setMainMenu:men_bar];

        NSMenuItem * men_item = 0;
        men_item = [NSMenuItem alloc];
        [men_item initWithTitle:@"" action:NULL keyEquivalent:@""];
        [men_item autorelease];
        
        [men_bar addItem:men_item];
        NSMenu * app_menu = [[NSMenu alloc] init];
        [app_menu autorelease];
        
        [men_item setSubmenu:app_menu];

        NSString * quit_title = 0;
        quit_title = [NSString stringWithFormat:@"Quit %@", app_name];
        NSMenuItem * quit_menu_item = 0;
        quit_menu_item = [NSMenuItem alloc];
        [quit_menu_item initWithTitle:quit_title
                action:@selector(terminate:) keyEquivalent:@"q"];
        [quit_menu_item autorelease];
        [app_menu addItem:quit_menu_item];

        macos_app_delegate * app_delegate = 0;
        app_delegate = [[[macos_app_delegate alloc] init] autorelease];
        
        [NSApp setDelegate:app_delegate];
        [NSApp run];

        CGFloat si[2] = {640, 360};
        NSRect view_rect = NSMakeRect(0, 0, si[0], si[1]);

        NSWindowStyleMask win_style =
                NSWindowStyleMaskTitled |
                NSWindowStyleMaskClosable |
                NSWindowStyleMaskMiniaturizable |
                NSWindowStyleMaskResizable;
        
        NSWindow * window = 0;
        window = [NSWindow alloc];
        [window initWithContentRect:view_rect
                styleMask:win_style
                backing:NSBackingStoreBuffered
                defer:NO];
        
        [window autorelease];
        [window cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
        [window setTitle:app_name];

        macos_window_delegate * window_delegate = 0;
        window_delegate = [macos_window_delegate alloc];
        [window_delegate init];
        [window_delegate autorelease];
        
        [window setDelegate:window_delegate];
        ctx->window_delegate = window_delegate;

        macos_metal_view *mview = 0;
        mview = [macos_metal_view alloc];
        [mview initWithFrame:view_rect];
        mview.wantsLayer       = YES;
        mview.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        ctx->metal_view        = mview;
        
        [window setContentView:ctx->metal_view];

        ctx->metal_layer = (CAMetalLayer *)[ctx->metal_view layer];
        ctx->metal_layer.device = desc->gpu_device;
        ctx->metal_layer.pixelFormat = ctx->metal_layer.pixelFormat;

        [NSApp activateIgnoringOtherApps:YES];
        [window makeKeyAndOrderFront:nil];

        *out = ctx;
    
        return DRV_APP_RESULT_OK;
}


drv_app_result
drv_app_ctx_destroy(
        struct drv_app_ctx **destroy)
{
        (void)destroy;
        
        return DRV_APP_RESULT_FAIL;
}


NSEvent *
process_next_evt(NSDate *date) {
        NSEvent *evt = 0;
        
        evt = [NSApp nextEventMatchingMask:NSEventMaskAny
                        untilDate:date
                        inMode:NSDefaultRunLoopMode
                        dequeue:YES];
        
        return evt;
}


drv_app_result
drv_app_ctx_process(
        struct drv_app_ctx *ctx,
        uint64_t *out_events)
{
        (void)out_events;
        
        /* app event process */
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        NSDate *past = [NSDate distantPast];
        
        NSEvent *evt = process_next_evt(past);
        
        while(evt) {
                [NSApp sendEvent:evt];
                evt = process_next_evt(past);
        }
        
        CGSize si;
        si.width  = 640.0;
        si.height = 360.0;
        ctx->metal_layer.drawableSize = si;

        [pool drain];
        pool = nil;

        if(ctx->window_delegate->try_close) {
            ctx->window_delegate->try_close = 0;
            return DRV_APP_RESULT_FAIL;
        }

        return DRV_APP_RESULT_OK;
}


drv_app_result
drv_app_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_data *data)
{
        data->layer = (void*)ctx->metal_layer;

        return DRV_APP_RESULT_OK;
}
