#ifndef DRV_PLAT_INCLUDED_22FADC07_0946_4F22_B0CC_985AC0A785E6
#define DRV_PLAT_INCLUDED_22FADC07_0946_4F22_B0CC_985AC0A785E6


#ifndef __cplusplus
#include <stdint.h>
#include <stddef.h>
#else
#include <cstdint>
#include <cstddef>
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct drv_app_ctx; /* opaque */


/* ----------------------------------------------------------- Identifiers -- */


typedef enum _drv_app_result {
        DRV_APP_RESULT_OK,
        DRV_APP_RESULT_BAD_PARAMS,
        DRV_APP_RESULT_INVALID_DESC,
        DRV_APP_RESULT_FAIL,
} drv_app_result;


typedef enum _drv_plat_event {
        DRV_APP_EVENT_SURFACE_MOVED = 1 << 0,
        DRV_APP_EVENT_SURFACE_CLOSED = 1 << 1,
        DRV_APP_EVENT_INPUT = 1 << 2,
        DRV_APP_EVENT_EXTERNAL = 1 << 3,
} drv_plat_event;


typedef void*(*drv_app_alloc_fn)(size_t);
typedef void(*drv_app_free_fn)(void *);


/* -------------------------------------------------------------- Lifetime -- */


typedef enum _drv_app_gpu_device_id {
        #ifdef _WIN32
        DRV_APP_GPU_DEVICE_DX12,
        #elif defined(__linux__)

        #elif defined(__APPLE__)
        DRV_APP_GPU_DEVICE_METAL,
        #endif
} drv_app_gpu_device_id;


struct drv_app_gpu_device {
        uint32_t id;
        uint32_t pad;
        uint8_t api_data[64];
};

/*
 * returns DRV_APP_RESULT_BAD_PARAMS if ctx is null,
 * returns DRV_APP_RESULT_BAD_PARAMS if device is not recognized.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_gpu_device_create(
        drv_app_gpu_device_id id,
        struct drv_app_gpu_device *out_device);


drv_app_result
drv_app_gpu_device_destroy(
        struct drv_app_gpu_device *device);


struct drv_app_ctx_create_desc {
        int width;
        int height;
        const char *title;
        struct drv_app_gpu_device *gpu_device;
        drv_app_alloc_fn alloc_fn;
        drv_app_free_fn free_fn;
};


/*
 * returns DRV_APP_RESULT_BAD_PARAMS if desc is null.
 * returns DRV_APP_RESULT_BAD_PARAMS if out_ctx is null.
 * returns DRV_APP_RESULT_INVALID_DESC if desc.gpu_device is null.
 * returns DRV_APP_RESULT_INVALID_DESC if desc.alloc_fn is null.
 * returns DRV_APP_RESULT_FAIL on internal failures.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_ctx_create(
        const struct drv_app_ctx_create_desc *desc,
        struct drv_app_ctx **out_ctx);


/*
 * returns DRV_APP_RESULT_BAD_PARAMS if destroy is null.
 * returns DRV_APP_RESULT_BAD_PARAMS if *destroy is null.
 * returns DRV_APP_RESULT_FAIL on internal failures.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_ctx_destroy(
        struct drv_app_ctx **destroy);


/*
 * returns DRV_APP_RESULT_BAD_PARAMS if ctx is null.
 * returns DRV_APP_RESULT_FAIL on internal failures.
 * returns DRV_APP_RESULT_FAIL if ctx is no longer valid.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_ctx_process(
        struct drv_app_ctx *ctx,
        uint64_t *out_events);


/* ----------------------------------------------------------------- Input -- */


typedef enum _drv_app_button_state {
        DRV_APP_BUTTON_STATE_UP         = 1 << 0,
        DRV_APP_BUTTON_STATE_UP_EVENT   = 1 << 1,
        DRV_APP_BUTTON_STATE_DOWN       = 1 << 2,
        DRV_APP_BUTTON_STATE_DOWN_EVENT = 1 << 3,
} drv_app_button_state;


typedef enum _drv_app_kb_id {
        /* dummy key */

        DRV_APP_KB_NULL,

        /* alphabet */

        DRV_APP_KB_A, DRV_APP_KB_B, DRV_APP_KB_C, DRV_APP_KB_D, DRV_APP_KB_E,
        DRV_APP_KB_F, DRV_APP_KB_G, DRV_APP_KB_H, DRV_APP_KB_I, DRV_APP_KB_J,
        DRV_APP_KB_K, DRV_APP_KB_L, DRV_APP_KB_M, DRV_APP_KB_N, DRV_APP_KB_O,
        DRV_APP_KB_P, DRV_APP_KB_Q, DRV_APP_KB_R, DRV_APP_KB_S, DRV_APP_KB_T,
        DRV_APP_KB_U, DRV_APP_KB_V, DRV_APP_KB_W, DRV_APP_KB_X, DRV_APP_KB_Y,
        DRV_APP_KB_Z,

        /* numbers */

        DRV_APP_KB_1, DRV_APP_KB_2, DRV_APP_KB_3, DRV_APP_KB_4, DRV_APP_KB_5,
        DRV_APP_KB_6, DRV_APP_KB_7, DRV_APP_KB_8, DRV_APP_KB_9, DRV_APP_KB_0,

        /* direction keys */

        DRV_APP_KB_UP, DRV_APP_KB_DOWN, DRV_APP_KB_LEFT, DRV_APP_KB_RIGHT,

        /* function keys */

        DRV_APP_KB_F1, DRV_APP_KB_F2,  DRV_APP_KB_F3,  DRV_APP_KB_F4,
        DRV_APP_KB_F5, DRV_APP_KB_F6,  DRV_APP_KB_F7,  DRV_APP_KB_F8,
        DRV_APP_KB_F9, DRV_APP_KB_F10, DRV_APP_KB_F11, DRV_APP_KB_F12,

        /* others */

        DRV_APP_KB_ESC, DRV_APP_KB_SPACE,
        DRV_APP_KB_LSHIFT, DRV_APP_KB_RSHIFT,
        DRV_APP_KB_LCTRL, DRV_APP_KB_RCTRL,

        /* count */

        DRV_APP_KB_COUNT

} drv_app_kb_id;


/*
 * returns DRV_APP_RESULT_BAD_PARAMS if ctx is null.
 * returns DRV_APP_RESULT_BAD_PARAMS if key_data is null.
 * returns DRV_APP_RESULT_FAIL on internal failures.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_input_kb_data_get(
        struct drv_app_ctx *ctx,
        uint8_t **key_data); /* Valid for lifetime of app */


typedef enum _drv_app_ms_key_id {
        DRV_APP_MS_KEY_LEFT,
        DRV_APP_MS_KEY_MIDDLE,
        DRV_APP_MS_KEY_RIGHT,

        /* count */

        DRV_APP_MS_KEY_COUNT
} drv_app_ms_key_id;


struct drv_app_mouse_data {
        float dx;
        float dy;

        float x;
        float y;

        uint8_t buttons[DRV_APP_MS_KEY_COUNT];
};

drv_app_result
drv_app_input_ms_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_mouse_data **ms_data); /* Valid for lifetime of app */


/* ------------------------------------------------------------------ Data -- */

#if _WIN32

struct drv_app_data_win32 {
        void *hwnd;
        struct ID3D12Device *dx_device;
        struct IDXGIFactory4 *dx_factory;
};


/*
 * returns DRV_APP_RESULT_BAD_PARAMS if ctx is null.
 * returns DRV_APP_RESULT_BAD_PARAMS if data is null.
 * returns DRV_APP_RESULT_FAIL on internal failures.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_data_get_win32(
        struct drv_app_ctx *ctx,
        struct drv_app_data_win32 *data);


#else if __APPLE__


struct drv_app_data_macos {
        void *metal_view;
        void *metal_device;
};


drv_app_result
drv_app_data_get_macos(
        struct drv_app_ctx *ctx,
        struct drv_app_data_macos *data);

#endif


#ifdef __cplusplus
} /* extern */
#endif


#endif
