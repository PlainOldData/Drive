
#include <drive/app.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_APP_PCHECK
#define DRV_APP_PCHECK 1
#endif


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


- (void)windowDidMove:(NSNotification *)notification
{
        (void)notification;
}


- (void)windowDidResize:(NSNotification *)notification
{
        (void)notification;
}

@end

@interface macos_metal_view : NSView {}
@property (nonatomic) struct drv_app_ctx *drv_ctx;
@end


/* -------------------------------------------------------------- Lifetime -- */


drv_app_result
drv_app_gpu_device(
        drv_app_gpu_device_id device,
        void **out_device)
{
        if(device == DRV_APP_GPU_DEVICE_METAL) {
                *out_device = MTLCreateSystemDefaultDevice();
                return DRV_APP_RESULT_OK;
        }
        
        return DRV_APP_RESULT_BAD_PARAMS;
}


struct drv_app_ctx {
        macos_window_delegate *window_delegate;
        CAMetalLayer * metal_layer;
        macos_metal_view * metal_view;
        void * gpu_device;
        drv_app_free_fn free_fn;
        
        uint64_t events;
        
        int width, height;
        
        size_t keycode_map[0xFF];
        uint8_t key_state[DRV_APP_KB_COUNT];
        struct drv_app_mouse_data ms_state;
        
        NSAutoreleasePool* app_pool;
};


@implementation macos_metal_view
- (BOOL)wantsUpdateLayer
{
        return YES;
}

- (BOOL)acceptsFirstResponder {
        return YES;
}

- (void)keyDown:(NSEvent *)event
{
        UInt16 kc = [event keyCode];
        uint8_t b = DRV_APP_BUTTON_STATE_DOWN_EVENT | DRV_APP_BUTTON_STATE_DOWN;
        
        drv_app_kb_id idx = [self drv_ctx]->keycode_map[kc];
        [self drv_ctx]->key_state[idx] = b;
        [self drv_ctx]->events |= DRV_APP_EVENT_INPUT;
}


- (void)keyUp:(NSEvent *)event
{
        UInt16 kc = [event keyCode];
        uint8_t b = DRV_APP_BUTTON_STATE_UP_EVENT | DRV_APP_BUTTON_STATE_UP;
        
        drv_app_kb_id idx = [self drv_ctx]->keycode_map[kc];
        [self drv_ctx]->key_state[idx] = b;
        [self drv_ctx]->events |= DRV_APP_EVENT_INPUT;
}

- (void)mouseMoved:(NSEvent *)event
{
        CGFloat dx = [event deltaX];
        CGFloat dy = [event deltaY];
        NSPoint pt = [event locationInWindow];
        
        [self drv_ctx]->ms_state.dx = dx;
        [self drv_ctx]->ms_state.dy = dy;
        [self drv_ctx]->ms_state.x  = pt.x;
        [self drv_ctx]->ms_state.y  = pt.y;
        
        [self drv_ctx]->events |= DRV_APP_EVENT_INPUT;
}

- (void)mouseDown:(NSEvent *)event
{
        (void)event;

        uint8_t b = DRV_APP_BUTTON_STATE_DOWN_EVENT | DRV_APP_BUTTON_STATE_DOWN;
        [self drv_ctx]->ms_state.buttons[DRV_APP_MS_KEY_LEFT] = b;
        [self drv_ctx]->events |= DRV_APP_EVENT_INPUT;
}

-(void)mouseUp:(NSEvent *)event
{
        (void)event;

        uint8_t b = DRV_APP_BUTTON_STATE_UP_EVENT | DRV_APP_BUTTON_STATE_UP;
        [self drv_ctx]->ms_state.buttons[DRV_APP_MS_KEY_LEFT] = b;
        [self drv_ctx]->events |= DRV_APP_EVENT_INPUT;
}

- (void)rightMouseDown:(NSEvent *)event
{
        (void)event;

        uint8_t b = DRV_APP_BUTTON_STATE_DOWN_EVENT | DRV_APP_BUTTON_STATE_DOWN;
        [self drv_ctx]->ms_state.buttons[DRV_APP_MS_KEY_RIGHT] = b;
        [self drv_ctx]->events |= DRV_APP_EVENT_INPUT;
}

-(void)rightMouseUp:(NSEvent *)event
{
        (void)event;

        uint8_t b = DRV_APP_BUTTON_STATE_UP_EVENT | DRV_APP_BUTTON_STATE_UP;
        [self drv_ctx]->ms_state.buttons[DRV_APP_MS_KEY_RIGHT] = b;
        [self drv_ctx]->events |= DRV_APP_EVENT_INPUT;
}

- (CALayer *)makeBackingLayer
{
        Class layer_class = NSClassFromString(@"CAMetalLayer");
        
        return [layer_class layer];
}
@end


drv_app_result
drv_app_ctx_create(
        const struct drv_app_ctx_create_desc *desc,
        struct drv_app_ctx **out_ctx)
{
        /* param checks */
        if(DRV_APP_PCHECK && !desc) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        if(DRV_APP_PCHECK && !out_ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        if(DRV_APP_PCHECK && !desc->alloc_fn) {
                assert(!"DRV_APP_RESULT_INVALID_DESC");
                return DRV_APP_RESULT_INVALID_DESC;
        }
        
        if(DRV_APP_PCHECK && !desc->gpu_device) {
                assert(!"DRV_APP_RESULT_INVALID_DESC");
                return DRV_APP_RESULT_INVALID_DESC;
        }
        
        struct drv_app_ctx *ctx = desc->alloc_fn(sizeof(*ctx));
        memset(ctx, 0, sizeof(*ctx));
        
        ctx->gpu_device = desc->gpu_device;
        ctx->width      = desc->width;
        ctx->height     = desc->height;
        ctx->free_fn    = desc->free_fn;
        
        /* app */
        ctx->app_pool = [[NSAutoreleasePool alloc] init];
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        /* name */
        const char *name = desc->title ? desc->title : "Drive";
        NSString *app_name = [NSString stringWithUTF8String:name];

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

        CGFloat si[2] = {(CGFloat)desc->width, (CGFloat)desc->height};
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
        [window setAcceptsMouseMovedEvents:YES];

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
        mview.drv_ctx = ctx;
        
        [window setContentView:ctx->metal_view];

        ctx->metal_layer             = (CAMetalLayer *)[ctx->metal_view layer];
        ctx->metal_layer.device      = desc->gpu_device;
        ctx->metal_layer.pixelFormat = ctx->metal_layer.pixelFormat;

        [NSApp activateIgnoringOtherApps:YES];
        [window makeKeyAndOrderFront:nil];

        /* input */
        /* https://stackoverflow.com/questions/3202629/where-can-i-find-a-list-of-mac-virtual-key-codes */
        
        ctx->keycode_map[0x00] = DRV_APP_KB_A;
        ctx->keycode_map[0x0B] = DRV_APP_KB_B;
        ctx->keycode_map[0x08] = DRV_APP_KB_C;
        ctx->keycode_map[0x02] = DRV_APP_KB_D;
        ctx->keycode_map[0x0E] = DRV_APP_KB_E;
        ctx->keycode_map[0x03] = DRV_APP_KB_F;
        ctx->keycode_map[0x05] = DRV_APP_KB_G;
        ctx->keycode_map[0x04] = DRV_APP_KB_H;
        ctx->keycode_map[0x22] = DRV_APP_KB_I;
        ctx->keycode_map[0x26] = DRV_APP_KB_J;
        ctx->keycode_map[0x28] = DRV_APP_KB_K;
        ctx->keycode_map[0x25] = DRV_APP_KB_L;
        ctx->keycode_map[0x2E] = DRV_APP_KB_M;
        ctx->keycode_map[0x2D] = DRV_APP_KB_N;
        ctx->keycode_map[0x1F] = DRV_APP_KB_O;
        ctx->keycode_map[0x23] = DRV_APP_KB_P;
        ctx->keycode_map[0x0C] = DRV_APP_KB_Q;
        ctx->keycode_map[0x0F] = DRV_APP_KB_R;
        ctx->keycode_map[0x01] = DRV_APP_KB_S;
        ctx->keycode_map[0x11] = DRV_APP_KB_T;
        ctx->keycode_map[0x20] = DRV_APP_KB_U;
        ctx->keycode_map[0x09] = DRV_APP_KB_V;
        ctx->keycode_map[0x0D] = DRV_APP_KB_W;
        ctx->keycode_map[0x07] = DRV_APP_KB_X;
        ctx->keycode_map[0x10] = DRV_APP_KB_Y;
        ctx->keycode_map[0x06] = DRV_APP_KB_Z;

        ctx->keycode_map[0x12] = DRV_APP_KB_1;
        ctx->keycode_map[0x13] = DRV_APP_KB_2;
        ctx->keycode_map[0x14] = DRV_APP_KB_3;
        ctx->keycode_map[0x15] = DRV_APP_KB_4;
        ctx->keycode_map[0x17] = DRV_APP_KB_5;
        ctx->keycode_map[0x16] = DRV_APP_KB_6;
        ctx->keycode_map[0x1A] = DRV_APP_KB_7;
        ctx->keycode_map[0x1C] = DRV_APP_KB_8;
        ctx->keycode_map[0x19] = DRV_APP_KB_9;
        ctx->keycode_map[0x1D] = DRV_APP_KB_0;

        ctx->keycode_map[0x7E] = DRV_APP_KB_UP;
        ctx->keycode_map[0x7D] = DRV_APP_KB_DOWN;
        ctx->keycode_map[0x7B] = DRV_APP_KB_LEFT;
        ctx->keycode_map[0x7C] = DRV_APP_KB_RIGHT;
        
        ctx->keycode_map[0x7A] = DRV_APP_KB_F1;
        ctx->keycode_map[0x78] = DRV_APP_KB_F2;
        ctx->keycode_map[0x63] = DRV_APP_KB_F3;
        ctx->keycode_map[0x76] = DRV_APP_KB_F4;
        ctx->keycode_map[0x60] = DRV_APP_KB_F5;
        ctx->keycode_map[0x61] = DRV_APP_KB_F6;
        ctx->keycode_map[0x62] = DRV_APP_KB_F7;
        ctx->keycode_map[0x64] = DRV_APP_KB_F8;
        ctx->keycode_map[0x65] = DRV_APP_KB_F9;
        ctx->keycode_map[0x6D] = DRV_APP_KB_F10;
        ctx->keycode_map[0x67] = DRV_APP_KB_F11;
        ctx->keycode_map[0x6F] = DRV_APP_KB_F12;
        
        ctx->keycode_map[0x35] = DRV_APP_KB_ESC;
        ctx->keycode_map[0x31] = DRV_APP_KB_SPACE;
        ctx->keycode_map[0x38] = DRV_APP_KB_LSHIFT;
        ctx->keycode_map[0x3C] = DRV_APP_KB_RSHIFT;
        ctx->keycode_map[0x3B] = DRV_APP_KB_LCTRL;
        ctx->keycode_map[0x3E] = DRV_APP_KB_RCTRL;
        
        int i;
        for(i = 0; i < DRV_APP_KB_COUNT; ++i) {
                ctx->key_state[i] = DRV_APP_BUTTON_STATE_UP;
        }
        
        for(i = 0; i < DRV_APP_MS_KEY_COUNT; ++i) {
                ctx->ms_state.buttons[i] = DRV_APP_BUTTON_STATE_UP;
        }

        *out_ctx = ctx;
    
        return DRV_APP_RESULT_OK;
}


drv_app_result
drv_app_ctx_destroy(
        struct drv_app_ctx **destroy)
{
        if(DRV_APP_PCHECK && !destroy) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        if(DRV_APP_PCHECK && !(*destroy)) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        struct drv_app_ctx *ctx = *destroy;
        
        [ctx->app_pool drain];
        
        if(ctx->free_fn) {
                ctx->free_fn(ctx);
        }
        
        *destroy = 0;
        
        return DRV_APP_RESULT_OK;
}


NSEvent *
process_next_evt(NSDate *date)
{
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
        if(DRV_APP_PCHECK && !ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        /* remove key events */
        int i;
        for(i = 0; i < DRV_APP_KB_COUNT; ++i) {
                ctx->key_state[i] &= ~(DRV_APP_BUTTON_STATE_UP_EVENT);
                ctx->key_state[i] &= ~(DRV_APP_BUTTON_STATE_DOWN_EVENT);
        }
        
        for(i = 0; i < DRV_APP_MS_KEY_COUNT; ++i) {
                ctx->ms_state.buttons[i] &= ~(DRV_APP_BUTTON_STATE_UP_EVENT);
                ctx->ms_state.buttons[i] &= ~(DRV_APP_BUTTON_STATE_DOWN_EVENT);
        }
        
        /* app event process */
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        NSDate *past = [NSDate distantPast];

        /* process events */
        NSEvent *evt = process_next_evt(past);
        
        while(evt) {
                [NSApp sendEvent:evt];
                evt = process_next_evt(past);
        }
        
        CGSize si;
        si.width  = ctx->width;
        si.height = ctx->height;
        ctx->metal_layer.drawableSize = si;

        [pool drain];
        pool = nil;

        if(ctx->window_delegate->try_close) {
                ctx->window_delegate->try_close = 0;
                return DRV_APP_RESULT_FAIL;
        }
        
        if(out_events) {
                *out_events = ctx->events;
        }
        
        ctx->events = 0;

        return DRV_APP_RESULT_OK;
}


drv_app_result
drv_app_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_data *data)
{
        if(DRV_APP_PCHECK && !ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        if(DRV_APP_PCHECK && !data) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        data->view = (void*)ctx->metal_layer;
        
        data->gpu_device = ctx->gpu_device;

        return DRV_APP_RESULT_OK;
}


/* ----------------------------------------------------------------- Input -- */


drv_app_result
drv_app_input_kb_data_get(
        struct drv_app_ctx *ctx,
        uint8_t **key_data)
{
        if(DRV_APP_PCHECK && !ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        if(DRV_APP_PCHECK && !key_data) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        *key_data = ctx->key_state;
        
        return DRV_APP_RESULT_OK;
}


drv_app_result
drv_app_input_ms_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_mouse_data **ms_data)
{
        if(DRV_APP_PCHECK && !ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        if(DRV_APP_PCHECK && !ms_data) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }
        
        *ms_data = &ctx->ms_state;
        
        return DRV_APP_RESULT_OK;
}
