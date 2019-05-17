#ifndef DRV_PLAT_INCLUDED_
#define DRV_PLAT_INCLUDED_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


struct drv_plat_ctx; /* opaque */


/* ----------------------------------------------------------- Identifiers -- */


typedef enum _drv_plat_result {
        DRV_PLAT_RESULT_OK,
        DRV_PLAT_RESULT_BAD_PARAMS,
        DRV_MEM_RESULT_INVALID_DESC,
} drv_plat_result;


typedef enum _drv_plat_event {
        DRV_PLAT_EVENT_WIN_MOVED = 1 << 0,
        DRV_PLAT_EVENT_WIN_CLOSED = 1 << 1,
        DRV_PLAT_EVENT_INPUT = 1 << 2,
        DRV_PLAT_EVENT_EXTERNAL = 1 << 3,
} drv_plat_event;


/* -------------------------------------------------------------- Lifetime -- */


typedef enum _drv_plat_gpu_device {
        DRV_PLAT_GPU_DEVICE_DX12,
        DRV_PLAT_GPU_DEVICE_VULKAN,
        DRV_PLAT_GPU_DEVICE_GL,
} drv_plat_gpu_device;


struct drv_plat_ctx_create_desc {
        drv_plat_gpu_device gpu_device;
};


drv_plat_result
drv_plat_ctx_create(
        const struct drv_plat_ctx_create_desc *desc,
        struct drv_plat_ctx **out);


drv_plat_result
drv_plat_ctx_destroy();


/* ---------------------------------------------------------------- Events -- */


drv_plat_result
drv_plat_event_poll(
        struct drv_plat_ctx *ctx,
        uint64_t *out_events);


/* ---------------------------------------------------------------- Window -- */


drv_plat_result
drv_plat_window(
        struct drv_plat_ctx *ctx);


/* ----------------------------------------------------------------- Input -- */


struct drv_keyboard_get_desc {
        int keyboard_id;
};


drv_plat_result
drv_plat_keyboard_get(
        struct drv_plat_ctx *ctx,
        uint8_t **out_key_state,
        int *out_key_count);


struct drv_mouse_get_desc {
        int mouse_id;
};


drv_plat_result
drv_plat_keyboard_get(
        struct drv_plat_ctx *ctx,
        uint8_t **out_key_state,
        int *out_key_count);



#ifdef __cplusplus
} /* extern */
#endif


#endif
