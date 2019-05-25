
#include <drive/app.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


/* -------------------------------------------------------------- Platform -- */


@interface macos_app_delegate : NSObject<NSApplicationDelegate> {}
@end

@implementation macos_app_delegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [NSApp stop:nil];
    NSEvent * event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
    [NSApp postEvent:event atStart:YES];
}
@end

@interface macos_window_delegate : NSObject<NSWindowDelegate> {
    @public uint32_t try_close;
}
@end

@implementation macos_window_delegate
- (BOOL)windowShouldClose:(NSWindow *)sender {
    try_close++;
    return NO;
}
@end

@interface macos_metal_view : NSView {}
@end

@implementation macos_metal_view
- (BOOL)wantsUpdateLayer {
    return YES;
}

- (CALayer *)makeBackingLayer {
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
        NSMenu * menu_bar = [[[NSMenu alloc] init] autorelease];
        [NSApp setMainMenu:menu_bar];

        NSMenuItem * app_menu_item = [[[NSMenuItem alloc] initWithTitle:@"" action:NULL keyEquivalent:@""] autorelease];
        [menu_bar addItem:app_menu_item];
        NSMenu * app_menu = [[[NSMenu alloc] init] autorelease];
        [app_menu_item setSubmenu:app_menu];

        NSString * quit_title = [NSString stringWithFormat:@"Quit %@", app_name];
        NSMenuItem * quit_menu_item = [[[NSMenuItem alloc] initWithTitle:quit_title action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
        [app_menu addItem:quit_menu_item];

        macos_app_delegate * app_delegate = [[[macos_app_delegate alloc] init] autorelease];
        [NSApp setDelegate:app_delegate];
        [NSApp run];

        float view_size[2] = { 640, 360 };
        NSRect view_rect = NSMakeRect(0, 0, (CGFloat)view_size[0], (CGFloat)view_size[1]);

        NSWindowStyleMask window_style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
        NSWindow * window = [[[NSWindow alloc] initWithContentRect:view_rect styleMask:window_style backing:NSBackingStoreBuffered defer:NO] autorelease];
        [window cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
        [window setTitle:app_name];

        macos_window_delegate * window_delegate = [[[macos_window_delegate alloc] init] autorelease];
        [window setDelegate:window_delegate];
    
        ctx->window_delegate = window_delegate;

        ctx->metal_view = [[macos_metal_view alloc] initWithFrame:view_rect];
        ctx->metal_view.wantsLayer = YES;
        ctx->metal_view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [window setContentView:ctx->metal_view];

        NSError * metal_err = nil;

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
        return DRV_APP_RESULT_FAIL;
}


drv_app_result
drv_app_ctx_process(
        struct drv_app_ctx *ctx,
        uint64_t *out_events)
{
        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

        while(1) {
            NSEvent * event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
            if(event != nil) {
                [NSApp sendEvent:event];
            }
            else {
                break;
            }
        }
        
        float view_size[2] = { 640, 360 };
        ctx->metal_layer.drawableSize = (CGSize) { (CGFloat)view_size[0], (CGFloat)view_size[1], };

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
