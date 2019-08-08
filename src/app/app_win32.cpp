#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <drive/app.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>
#include <Shellapi.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_APP_PCHECKS
#define DRV_APP_PCHECKS 1
#endif


/* -------------------------------------------------------------- Lifetime -- */


struct drv_app_ctx {
        HWND hwnd;
        HINSTANCE hinstance;
        HDC hdc;

        uint64_t events;

        /* input */

        drv_app_kb_id keycode_map[0xFF];
        uint8_t key_state[DRV_APP_KB_COUNT];
};


void
internal_destroy_window(
        struct drv_app_ctx *ctx)
{
        /*
        * These need to be wrapped in checks, because this function can get
        * get called multiple times, and DestroyWindow sends
        * WM_DESTROY event.
        */

        if(ctx->hwnd && ctx->hdc) {
                ReleaseDC(ctx->hwnd, ctx->hdc);
                ctx->hdc = 0;
        }

        if(ctx->hwnd) {
                DestroyWindow(ctx->hwnd);
                ctx->hwnd = 0;
        }
}


LRESULT CALLBACK
internal_wnd_proc(HWND hWnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
        LRESULT result = 0;

        drv_app_ctx *ctx = (drv_app_ctx*)GetPropA(hWnd, "drive_app_ctx");

        switch (u_msg) {
        case WM_CLOSE:
        case WM_QUIT:
        case WM_DESTROY: {
                internal_destroy_window(ctx);

                break;
        }
        case WM_SIZING: {
                int i = 0;
                break;
        }
        case WM_SIZE: {
                if(ctx) {
                        /*ctx->resized = 1;
                        ctx->width = (int)LOWORD(l_param);
                        ctx->height = (int)HIWORD(l_param);*/
                }
                break;
        }
        case WM_MOUSEMOVE: {
                if (ctx) {
                        /*int curr_x = ctx->ms_x;
                        int curr_y = ctx->ms_y;

                        ctx->ms_x = (int)LOWORD(l_param);
                        ctx->ms_y = (int)HIWORD(l_param);

                        ctx->ms_dx = (float)curr_x - ctx->ms_x;
                        ctx->ms_dy = (float)curr_y - ctx->ms_y;*/
                }
                break;
        }
        case WM_MOUSEWHEEL: {
                if (ctx) {
                        /*short scroll = HIWORD(w_param);
                        ctx->ms_scroll_y = (float)scroll / (float)WHEEL_DELTA;*/
                }
                break;
        }
        case WM_LBUTTONDOWN: {
                if (ctx) {
                        //ctx->ms_keys[RTOR_MS_LEFT] = RTOR_KEY_PRESS_DOWN;
                }
                break;
        }
        case WM_RBUTTONDOWN: {
                if (ctx) {
                        //ctx->ms_keys[RTOR_MS_RIGHT] = RTOR_KEY_PRESS_DOWN;
                }
                break;
        }
        case WM_MBUTTONDOWN: {
                if (ctx) {
                        //ctx->ms_keys[RTOR_MS_MIDDLE] = RTOR_KEY_PRESS_DOWN;
                }
                break;
        }
        case WM_LBUTTONUP: {
                if (ctx) {
                        //ctx->ms_keys[RTOR_MS_LEFT] = RTOR_KEY_PRESS_UP;
                }
                break;
        }
        case WM_RBUTTONUP: {
                if (ctx) {
                        //ctx->ms_keys[RTOR_MS_RIGHT] = RTOR_KEY_PRESS_UP;
                }
                break;
        }
        case WM_MBUTTONUP: {
                if (ctx) {
                        //ctx->ms_keys[RTOR_MS_MIDDLE] = RTOR_KEY_PRESS_UP;
                }
                break;
        }
        case WM_MOVING: {
                int i = 0;
                break;
        }

        case WM_KEYDOWN: {
                if(ctx) {
                        SHORT kc = (SHORT)w_param;
                        uint8_t b =
                                DRV_APP_BUTTON_STATE_DOWN_EVENT |
                                DRV_APP_BUTTON_STATE_DOWN;

                        size_t idx = ctx->keycode_map[kc];

                        if(idx < DRV_APP_KB_COUNT) {
                                ctx->key_state[idx] = b;
                                ctx->events |= DRV_APP_EVENT_INPUT;
                        }
                }

                break;
        }

        case WM_KEYUP: {
                if(ctx) {
                        SHORT kc = (SHORT)w_param;
                        uint8_t b =
                                DRV_APP_BUTTON_STATE_UP_EVENT |
                                DRV_APP_BUTTON_STATE_UP;

                        drv_app_kb_id idx = (drv_app_kb_id)ctx->keycode_map[kc];
                        
                        if(idx < DRV_APP_KB_COUNT) {
                                ctx->key_state[idx] = b;
                                ctx->events |= DRV_APP_EVENT_INPUT;
                        }
                }

                break;
        }

        case WM_CHAR: {
                if(ctx) {
                        //SHORT c16 = (SHORT)w_param;
                        //if(c16 == '\r') {
                        //        c16 = '\n';
                        //}

                        ///* Do proper UTF-16 to UTF-8 conversion!! */
                        //if((c16 > 31 && c16 < 127) || c16 == '\b' || c16 == '\n') {
                        //        if(ctx->char_count < (RTOR_KEYBOARD_CHAR_COUNT_MAX - 1)) {
                        //                char c = (char)c16;

                        //                ctx->chars[ctx->char_count++] = c;
                        //                ctx->chars[ctx->char_count] = 0;
                        //        }
                        //}


                }

                break;
        }

        default: {
                result = DefWindowProcA(hWnd, u_msg, w_param, l_param);
                break;
        }
        }

        return result;
}


drv_app_result
drv_app_ctx_create(
        const struct drv_app_ctx_create_desc *desc,
        struct drv_app_ctx **out)
{
        /* param check */

        if(DRV_APP_PCHECKS && !desc) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        if(DRV_APP_PCHECKS && !out) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        /* alloc ctx */

        drv_app_alloc_fn alloc_fn = malloc;
        drv_app_free_fn free_fn = free;

        if (desc->alloc_fn) {
                alloc_fn = desc->alloc_fn;
                free_fn  = desc->free_fn;
        }

        struct drv_app_ctx *ctx = (struct drv_app_ctx*)alloc_fn(sizeof(*ctx));

        /* register class */

        WNDCLASSA wc;
        memset(&wc, 0, sizeof(wc));

        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc   = (WNDPROC)internal_wnd_proc;
        wc.hInstance     = ctx->hinstance;
        wc.lpszClassName = "Drive";
        wc.hCursor       = LoadCursorA(0, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        if(!RegisterClass(&wc)) {
                if(free_fn) {
                        free_fn(ctx);
                        ctx = 0;
                }

                assert(!"DRV_APP_RESULT_FAIL");
                return DRV_APP_RESULT_FAIL;
        }

        DWORD dw_ex_style = WS_EX_ACCEPTFILES;
        DWORD dw_style =
                WS_OVERLAPPEDWINDOW |
                WS_CLIPCHILDREN |
                WS_CLIPSIBLINGS;

        RECT rect;
        rect.left   = rect.top = 0;
        rect.right  = desc->width;
        rect.bottom = desc->height;

        AdjustWindowRectEx(&rect, dw_style, FALSE, dw_ex_style);

        /* create window */

        ctx->hwnd = CreateWindowExA(
                dw_ex_style,
                wc.lpszClassName,
                desc->title,
                dw_style,
                0, 0,
                rect.right - rect.left,
                rect.bottom - rect.top,
                NULL,
                NULL,
                ctx->hinstance,
                NULL
        );

        if(!ctx->hwnd) {
                if(free_fn) {
                        free_fn(ctx);
                }

                assert(!"DRV_APP_RESULT_FAIL");
                return DRV_APP_RESULT_FAIL;
        }

        if(ctx->hwnd) {
                ctx->hdc = GetDC(ctx->hwnd);
        }

        /* input */
        /* https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes */

        int k;

        for(k = 0; k < 0xFF; ++k) {
                ctx->keycode_map[k] = DRV_APP_KB_COUNT;
        }

        ctx->keycode_map[0x41] = DRV_APP_KB_A;
        ctx->keycode_map[0x42] = DRV_APP_KB_B;
        ctx->keycode_map[0x43] = DRV_APP_KB_C;
        ctx->keycode_map[0x44] = DRV_APP_KB_D;
        ctx->keycode_map[0x45] = DRV_APP_KB_E;
        ctx->keycode_map[0x46] = DRV_APP_KB_F;
        ctx->keycode_map[0x47] = DRV_APP_KB_G;
        ctx->keycode_map[0x48] = DRV_APP_KB_H;
        ctx->keycode_map[0x49] = DRV_APP_KB_I;
        ctx->keycode_map[0x4A] = DRV_APP_KB_J;
        ctx->keycode_map[0x4B] = DRV_APP_KB_K;
        ctx->keycode_map[0x4C] = DRV_APP_KB_L;
        ctx->keycode_map[0x4D] = DRV_APP_KB_M;
        ctx->keycode_map[0x4E] = DRV_APP_KB_N;
        ctx->keycode_map[0x4F] = DRV_APP_KB_O;
        ctx->keycode_map[0x50] = DRV_APP_KB_P;
        ctx->keycode_map[0x51] = DRV_APP_KB_Q;
        ctx->keycode_map[0x52] = DRV_APP_KB_R;
        ctx->keycode_map[0x53] = DRV_APP_KB_S;
        ctx->keycode_map[0x54] = DRV_APP_KB_T;
        ctx->keycode_map[0x55] = DRV_APP_KB_U;
        ctx->keycode_map[0x56] = DRV_APP_KB_V;
        ctx->keycode_map[0x57] = DRV_APP_KB_W;
        ctx->keycode_map[0x58] = DRV_APP_KB_X;
        ctx->keycode_map[0x59] = DRV_APP_KB_Y;
        ctx->keycode_map[0x5A] = DRV_APP_KB_Z;

        ctx->keycode_map[0x31] = DRV_APP_KB_1;
        ctx->keycode_map[0x32] = DRV_APP_KB_2;
        ctx->keycode_map[0x33] = DRV_APP_KB_3;
        ctx->keycode_map[0x34] = DRV_APP_KB_4;
        ctx->keycode_map[0x35] = DRV_APP_KB_5;
        ctx->keycode_map[0x36] = DRV_APP_KB_6;
        ctx->keycode_map[0x37] = DRV_APP_KB_7;
        ctx->keycode_map[0x38] = DRV_APP_KB_8;
        ctx->keycode_map[0x39] = DRV_APP_KB_9;
        ctx->keycode_map[0x30] = DRV_APP_KB_0;

        ctx->keycode_map[0x26] = DRV_APP_KB_UP;
        ctx->keycode_map[0x28] = DRV_APP_KB_DOWN;
        ctx->keycode_map[0x25] = DRV_APP_KB_LEFT;
        ctx->keycode_map[0x27] = DRV_APP_KB_RIGHT;

        ctx->keycode_map[0x70] = DRV_APP_KB_F1;
        ctx->keycode_map[0x71] = DRV_APP_KB_F2;
        ctx->keycode_map[0x72] = DRV_APP_KB_F3;
        ctx->keycode_map[0x73] = DRV_APP_KB_F4;
        ctx->keycode_map[0x74] = DRV_APP_KB_F5;
        ctx->keycode_map[0x75] = DRV_APP_KB_F6;
        ctx->keycode_map[0x76] = DRV_APP_KB_F7;
        ctx->keycode_map[0x77] = DRV_APP_KB_F8;
        ctx->keycode_map[0x78] = DRV_APP_KB_F9;
        ctx->keycode_map[0x79] = DRV_APP_KB_F10;
        ctx->keycode_map[0x7A] = DRV_APP_KB_F11;
        ctx->keycode_map[0x7B] = DRV_APP_KB_F12;

        ctx->keycode_map[0x1B] = DRV_APP_KB_ESC;
        ctx->keycode_map[0x20] = DRV_APP_KB_SPACE;
        ctx->keycode_map[0xA0] = DRV_APP_KB_LSHIFT;
        ctx->keycode_map[0xAA] = DRV_APP_KB_RSHIFT;
        ctx->keycode_map[0xA2] = DRV_APP_KB_LCTRL;
        ctx->keycode_map[0xA3] = DRV_APP_KB_RCTRL;

        int i;
        for(i = 0; i < DRV_APP_KB_COUNT; ++i) {
                ctx->key_state[i] = DRV_APP_BUTTON_STATE_UP;
        }

        for(i = 0; i < DRV_APP_MS_KEY_COUNT; ++i) {
                //ctx->ms_state.buttons[i] = DRV_APP_BUTTON_STATE_UP;
        }

        /* display window */

        ShowWindow(ctx->hwnd, SW_SHOW);
        SetForegroundWindow(ctx->hwnd);
        SetFocus(ctx->hwnd);
        SetPropA(ctx->hwnd, "drive_app_ctx", ctx);

        /* finish up */

        *out = ctx;

        return DRV_APP_RESULT_OK;
}


drv_app_result
drv_app_ctx_destroy(
        struct drv_app_ctx **destroy)
{
        if(DRV_APP_PCHECKS && !destroy) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        return DRV_APP_RESULT_OK;
}


drv_app_result
drv_app_ctx_process(
        struct drv_app_ctx *ctx,
        uint64_t *out_events)
{
        /* param checks */

        if(DRV_APP_PCHECKS && !ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        /* setup */

        ctx->events = 0;

        /* remove key events */

        int i;
        for(i = 0; i < DRV_APP_KB_COUNT; ++i) {
                ctx->key_state[i] &= ~(DRV_APP_BUTTON_STATE_UP_EVENT);
                ctx->key_state[i] &= ~(DRV_APP_BUTTON_STATE_DOWN_EVENT);
        }

        for(i = 0; i < DRV_APP_MS_KEY_COUNT; ++i) {
                //ctx->ms_state.buttons[i] &= ~(DRV_APP_BUTTON_STATE_UP_EVENT);
                //ctx->ms_state.buttons[i] &= ~(DRV_APP_BUTTON_STATE_DOWN_EVENT);
        }

        /* process messages */

        MSG msg;

        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                switch (msg.message) {
                /*
                case WM_DROPFILES: {
                        internal_wm_drop_files(
                                ctx,
                                &msg);
                        break;
                }
                */

                default: {
                        TranslateMessage(&msg);
                        DispatchMessageA(&msg);
                        break;
                }
                }
        }

        /* return success */

        if(out_events) {
                *out_events = ctx->events;
        }

        return ctx->hwnd ? DRV_APP_RESULT_OK : DRV_APP_RESULT_FAIL;
}


/* ------------------------------------------------------------------- App -- */


drv_app_result
drv_app_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_data *data)
{
        data->hwnd = ctx->hwnd;

        return DRV_APP_RESULT_OK;
}

drv_app_result
drv_app_gpu_device(
        drv_app_gpu_device_id device,
        void **out_device)
{
        /* param checks */

        if(DRV_APP_PCHECKS && device != DRV_APP_GPU_DEVICE_DX12) {
                assert(!"DRV_APP_RESULT_FAIL");
                return DRV_APP_RESULT_FAIL;
        }

        if(DRV_APP_PCHECKS && !out_device) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        /* d3d device */

        static ID3D12Device* dev = 0;

        //TODO(Albert): Create DX device!!
#if 0
        if(!dev) {
                D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_12_1;
                HRESULT res = D3D12CreateDevice(
                        NULL,
                        fl,
                        __uuidof(ID3D12Device),
                        (void**)&dev);

                if(FAILED(res)) {
                        assert(!"DRV_APP_RESULT_FAIL");
		        return DRV_APP_RESULT_FAIL;
	        }
        }

        if(!dev) {
                assert(!"DRV_APP_RESULT_FAIL");
		return DRV_APP_RESULT_FAIL;
        }
#endif

        /* return */

        *out_device = dev;

        return DRV_APP_RESULT_OK;
}


/* ----------------------------------------------------------------- Input -- */


drv_app_result
drv_app_input_kb_data_get(
        struct drv_app_ctx *ctx,
        uint8_t **key_data)
{
        if(DRV_APP_PCHECKS && !ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        if(DRV_APP_PCHECKS && !key_data) {
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
        if(DRV_APP_PCHECKS && !ctx) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        if(DRV_APP_PCHECKS && !ms_data) {
                assert(!"DRV_APP_RESULT_BAD_PARAMS");
                return DRV_APP_RESULT_BAD_PARAMS;
        }

        //*ms_data = &ctx->ms_state;

        return DRV_APP_RESULT_FAIL;
}


#endif
