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


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_APP_PCHECKS
#define DRV_APP_PCHECKS 1
#endif


/* -------------------------------------------------------------- Lifetime -- */


struct drv_app_ctx {
        HWND hwnd;
        HINSTANCE hinstance;
        HDC hdc;
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
        const struct drv_plat_ctx_create_desc *desc,
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
                //ctx->glrc = win32_create_gl_context(ctx->hwnd, ctx->dc);
        }

        /* display window */
        ShowWindow(ctx->hwnd, SW_SHOW);
        SetForegroundWindow(ctx->hwnd);
        SetFocus(ctx->hwnd);
        SetPropA(ctx->hwnd, "drive_app_ctx", ctx);

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
        uint64_t events = 0;

        /* process messages */
        MSG msg;

        /* handle messages we care about! */
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
        return ctx->hwnd ? DRV_APP_RESULT_OK : DRV_APP_RESULT_FAIL;
}


/* ------------------------------------------------------------------- App -- */


drv_app_result
drv_app_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_data *data)
{
        data->hwnd = ctx->hwnd;
}

#endif
