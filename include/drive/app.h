#ifndef DRV_PLAT_INCLUDED_22FADC07_0946_4F22_B0CC_985AC0A785E6
#define DRV_PLAT_INCLUDED_22FADC07_0946_4F22_B0CC_985AC0A785E6


#include <stdint.h>
#include <stddef.h>


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


typedef enum _drv_plat_gpu_device {
        #ifdef _WIN32
        DRV_PLAT_GPU_DEVICE_DX12,
        DRV_PLAT_GPU_DEVICE_VULKAN,
        #elif defined(__linux__)
        DRV_PLAT_GPU_DEVICE_VULKAN,
        #elif defined(__APPLE__)
        DRV_PLAT_GPU_DEVICE_METAL,
        #endif
} drv_plat_gpu_device;


struct drv_plat_ctx_create_desc {
        int width;
        int height;
        const char *title;
        void *gpu_device;
        drv_app_alloc_fn alloc_fn;
        drv_app_free_fn free_fn;
};


drv_app_result
drv_app_ctx_create(
        const struct drv_plat_ctx_create_desc *desc,
        struct drv_app_ctx **out);


drv_app_result
drv_app_ctx_destroy(
        struct drv_app_ctx **destroy);


drv_app_result
drv_app_ctx_process(
        struct drv_app_ctx *ctx,
        uint64_t *out_events);


/* ------------------------------------------------------------------- App -- */


#ifdef _WIN32
struct drv_app_data {
        void* hwnd;
};
#endif

#ifdef __APPLE__
struct drv_app_data {
        void *layer;
};
#endif


drv_app_result
drv_app_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_data *data);


/* ---------------------------------------------------------------- Events -- */


drv_app_result
drv_plat_event_poll(
        struct drv_plat_ctx *ctx,
        uint64_t *out_events);


/* ---------------------------------------------------------------- Window -- */


drv_app_result
drv_plat_surface(
        struct drv_plat_ctx *ctx);



#ifdef __cplusplus
} /* extern */
#endif


#endif
