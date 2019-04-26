#ifndef DRV_SHED_INCLUED_7CC58AF1_1498_4386_AF93_3BB20D309150
#define DRV_SHED_INCLUED_7CC58AF1_1498_4386_AF93_3BB20D309150


#include <stdint.h>


#ifdef _cplusplus
extern "C" {
#endif


struct drv_shed_ctx; /* opaque type */


typedef enum _drv_shed_result {
        DRV_SHED_RESULT_OK,
        DRV_SHED_RESULT_BAD_PARAM,
        DRV_SHED_RESULT_INVALID_DESC,
        DRV_SHED_RESULT_FAIL,
} drv_shed_result;


/* -------------------------------------------------------------- Lifetime -- */


typedef void*(*drv_shd_alloc_fn)(unsigned long);
typedef void(*drv_shed_free_fn)(void *);
typedef void(*drv_log_fn)(const char *);


struct drv_shed_ctx_create_desc {
        int thread_count;             /* if 0 - then count = (cores - 2) */
        int thread_pin;               /* if 1 - then pin threads to cores */
        int thread_names;             /* if 1 - then threads named 'DRV' */
        drv_shd_alloc_fn shed_alloc;  /* required */
        drv_log_fn shed_log;          /* optional - must be built with DRV_SHED_LOGGING = 1*/
};


/*
 * returns DRV_SHED_RESULT_BAD_PARAM if desc is null.
 * returns DRV_SHED_RESULT_BAD_PARAM if out is null.
 * returns DRV_SHED_RESULT_INVALID_DESC if desc.shed_alloc is null.
 */
drv_shed_result
drv_shed_ctx_create(
        const struct drv_shed_ctx_create_desc *desc,    /* required */
        struct drv_shed_ctx **out);                     /* required */


struct drv_shed_ctx_destroy_desc {
        drv_shed_free_fn shed_free;
};


drv_shed_result
drv_shed_ctx_destroy(
        struct drv_shed_ctx **destroy);                 /* required */


drv_shed_result
drv_shed_ctx_join(
        struct drv_shed_ctx *ctx);                      /* required */


/* --------------------------------------------------------------- Enqueue -- */


typedef void(*drv_task_fn)(struct drv_shed_ctx*, void*);


struct drv_shed_work_desc {
        drv_task_fn func;       /* required */
        void *arg;              /* optional - gets passed to task */
        int tid_lock;           /* stop task being executed on other threads */
};


struct drv_shed_enqueue_desc {
        struct drv_shed_work_desc *work;                /* required */
        int work_count;                                 /* required - must be greater than 0 */
        uint64_t blocked_mk;                            /* optional - zero if none */
};


drv_shed_result
drv_shed_enqueue(
        struct drv_shed_ctx *ctx,                       /* required */
        const struct drv_shed_enqueue_desc *desc,       /* required */
        uint64_t *batch_mk);                            /* optional */


drv_shed_result
drv_shed_wait(
        struct drv_shed_ctx *ctx,                       /* required */
        uint64_t wait_mk);                              /* required */


/* --------------------------------------------------------------- Details -- */


typedef enum _drv_detail {
        DRV_SHED_MAX_JOBS,
        DRV_SHED_MAX_MARKERS,
        DRV_SHED_MAX_FIBERS,
        DRV_SHED_MAX_CORE_COUNT,
        DRV_SHED_MAX_PROFILE_STORE_BYTES,
        DRV_SHED_THREAD_COUNT,
        DRV_SHED_PEDANTIC_CHECKS,
} drv_detail;


drv_shed_result
drv_shed_detail_get(
        struct drv_shed_ctx *ctx,
        drv_detail detail,
        int *out);


/* -------------------------------------------------------------- Debuging -- */
/*
 *  drv_shed can keep profiling information, you can use the OS's API to resolve
 *  the function ptr to a symbol name.
 */


struct drv_shed_profile_data {
        void *func;
        unsigned long long start;
        unsigned long long end;
        int thread_id;
};


drv_shed_result
drv_shed_profile_data_get(
        struct drv_shed_ctx *ctx,
        struct drv_shed_profile_data *out_data,
        int *out_count);


#ifdef _cplusplus
} /* extern "C" */
#endif

#endif
