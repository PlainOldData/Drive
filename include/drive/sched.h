#ifndef DRV_SCHED_INCLUED_7CC58AF1_1498_4386_AF93_3BB20D309150
#define DRV_SCHED_INCLUED_7CC58AF1_1498_4386_AF93_3BB20D309150


#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


struct drv_sched_ctx; /* opaque type */


typedef enum _drv_sched_result {
        DRV_SCHED_RESULT_OK,
        DRV_SCHED_RESULT_BAD_PARAM,
        DRV_SCHED_RESULT_INVALID_DESC,
        DRV_SCHED_RESULT_OUT_OF_RESOURCES,
        DRV_SCHED_RESULT_BAD_CALL,
        DRV_SCHED_RESULT_FAIL,
} drv_sched_result;


/* -------------------------------------------------------------- Lifetime -- */


typedef enum _drv_sched_log_type {
        DRV_SCHED_LOG_ERROR = 0,
        DRV_SCHED_LOG_WARNING = 1,
        DRV_SCHED_LOG_INFO = 2,
} drv_sched_log_type;


typedef void*(*drv_sched_alloc_fn)(size_t);
typedef void(*drv_sched_free_fn)(void *);
typedef void(*drv_sched_log_fn)(drv_sched_log_type, const char *, void*);

/* internal profiling hooks */
typedef void(*drv_sched_profile_start_fn)(const char *func_name, void*ud);
typedef void(*drv_sched_profile_end_fn)(const char *func_name, void*ud);

/* task profiling hooks */
typedef void(*drv_sched_profile_task_started_fn)(void *fn, void*ud);
typedef void(*drv_sched_profile_task_ended_fn)(void *fn, void*ud);


struct drv_sched_ctx_create_desc {
        int thread_count;               /* if 0 - then count = (cores - 2) */
        int thread_pin;                 /* if 1 - then set thread affinity */
        const char *thread_name;        /* set name of thread */
        drv_sched_alloc_fn sched_alloc; /* required */
        
        drv_sched_log_fn log_fn;        /* optional - buit with DRV_SCHED_LOGGING = 1 */
        void *log_ud;                   /* optional - passed to logging cb */
        drv_sched_log_type log_level;   /* 0 for only errors, 1 for warnings and errors, 2 for warnings errors, and info */

        drv_sched_profile_start_fn prof_start;
        drv_sched_profile_end_fn prof_end;
        void *prof_ud;

        drv_sched_profile_task_started_fn task_start;
        drv_sched_profile_task_ended_fn task_end;
        void *task_ud;
};


/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if desc is null.
 * returns DRV_SCHED_RESULT_BAD_PARAM if out is null.
 * returns DRV_SCHED_RESULT_INVALID_DESC if desc.sched_alloc is null.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_sched_ctx_create(
        const struct drv_sched_ctx_create_desc *desc,    /* required */
        struct drv_sched_ctx **out);                     /* required */


struct drv_sched_ctx_destroy_desc {
        drv_sched_free_fn sched_free;                    /* optional */
        struct drv_sched_ctx **ctx_to_destroy;           /* required */
};


/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if desc is null.
 * returns DRV_SCHED_RESULT_INVALID_DESC if desc.ctx_to_destroy is null.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_sched_ctx_destroy(
        struct drv_sched_ctx_destroy_desc *desc);        /* required */


/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_BAD_CALL if not called on main thread.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_sched_ctx_join(
        struct drv_sched_ctx *ctx);                      /* required */


/* --------------------------------------------------------------- Enqueue -- */


typedef void(*drv_task_fn)(struct drv_sched_ctx*, void*);


struct drv_sched_work_desc {
        drv_task_fn func;       /* required */
        void *arg;              /* optional - gets passed to task */
        int tid_lock;           /* stop task being executed on other threads */
};


struct drv_sched_enqueue_desc {
        struct drv_sched_work_desc *work; /* required */
        int work_count;                   /* required - must be > 0 */
        uint64_t blocked_mk;              /* optional - zero if none */
};

/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_SCHED_RESULT_BAD_PARAM if desc is null.
 * returns DRV_SCHED_RESULT_INVALID_DESC if desc.work is null.
 * returns DRV_SCHED_RESULT_INVALID_DESC if desc.work_count is less than zero.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_sched_enqueue(
        struct drv_sched_ctx *ctx,                       /* required */
        const struct drv_sched_enqueue_desc *desc,       /* required */
        uint64_t *out_batch_mk);                         /* optional */


/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_sched_wait(
        struct drv_sched_ctx *ctx,                       /* required */
        uint64_t wait_mk);                              /* required */


/* --------------------------------------------------------------- Details -- */


typedef enum _drv_detail {
        DRV_SCHED_CTX_DETAIL_MAX_JOBS,
        DRV_SCHED_CTX_DETAIL_MAX_MARKERS,
        DRV_SCHED_CTX_DETAIL_MAX_FIBERS,
        DRV_SCHED_CTX_DETAIL_THREAD_COUNT,      /* including main thread */
        DRV_SCHED_CTX_DETAIL_PEDANTIC_CHECKS,   /* 1 for yes - 0 for no */
} drv_ctx_detail;


/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_SCHED_RESULT_BAD_PARAM if out is null.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_sched_ctx_detail_get(
        struct drv_sched_ctx *ctx,
        drv_ctx_detail detail,
        int *out);


typedef enum _drv_sys_detail {
        DRV_SCHED_SYS_DETAIL_PHYSICAL_CORE_COUNT,
        DRV_SCHED_SYS_DETAIL_LOGICAL_CORE_COUNT,
} drv_sys_detail;


/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if out is null.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_shed_sys_detail_get(
        drv_sys_detail detail,
        int *out);


/* -------------------------------------------------------------- Debuging -- */
/*
 *  drv_sched can keep profiling information, you can use the OS's API to
 *  resolve the function ptr to a symbol name.
 */


struct drv_sched_profile_data {
        void *func;
        unsigned long long start;
        unsigned long long end;
        int thread_id;
};


/*
 * returns DRV_SCHED_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_SCHED_RESULT_BAD_PARAM if out_data is null.
 * returns DRV_SCHED_RESULT_BAD_PARAM if out_count is null.
 * returns DRV_SCHED_RESULT_FAIL on internal failure.
 * returns DRV_SCHED_RESULT_OK on success.
 */
drv_sched_result
drv_sched_profile_data_get(
        struct drv_sched_ctx *ctx,
        struct drv_sched_profile_data *out_data,
        int *out_count);


/* ------------------------------------------------------------------ Util -- */
/*
 * Fibers can be woken up on different threads so a spinlock is a better choice.
 * This might not be the best/fastest option depending on your platform.
 */

typedef union _drv_spin_lock {
	void* align;
	int val;
} drv_spin_lock;


void
drv_spin_lock_init(
        int *lock); /* required - aligned int, see `drv_spin_lock` */


void
drv_spin_lock_aquire(
        int *lock); /* required - aligned int, see `drv_spin_lock` */

/*
 * returns 1 if a lock was aquired, 0 otherwise.
 */
int
drv_spin_lock_try_aquire(
        int *lock); /* required - aligned int, see `drv_spin_lock` */

void
drv_spin_lock_release(
        int *lock); /* required - aligned int, see `drv_spin_lock` */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
