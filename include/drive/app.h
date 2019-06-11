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
        DRV_APP_GPU_DEVICE_VULKAN,
        #elif defined(__linux__)
        DRV_APP_GPU_DEVICE_VULKAN,
        #elif defined(__APPLE__)
        DRV_APP_GPU_DEVICE_METAL,
        #endif
} drv_app_gpu_device_id;


/*
 * returns DRV_APP_RESULT_BAD_PARAMS if device is not recognized.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_gpu_device(
        drv_app_gpu_device_id device,
        void **out_device);


struct drv_app_ctx_create_desc {
        int width;
        int height;
        const char *title;
        void *gpu_device;
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


/* ------------------------------------------------------------------ Data -- */


#ifdef _WIN32
struct drv_app_data {
        void *hwnd;
        void *gpu_device;
};
#endif

#ifdef __APPLE__
struct drv_app_data {
        void *layer;
        void *gpu_device;
};
#endif


/*
 * returns DRV_APP_RESULT_BAD_PARAMS if ctx is null.
 * returns DRV_APP_RESULT_BAD_PARAMS if ctx is null.
 * returns DRV_APP_RESULT_FAIL on internal failures.
 * returns DRV_APP_RESULT_OK on success.
 */
drv_app_result
drv_app_data_get(
        struct drv_app_ctx *ctx,
        struct drv_app_data *data);


#ifdef __cplusplus
} /* extern */
#endif


#endif
