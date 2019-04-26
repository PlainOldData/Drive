#include <drive/sched.h>
#include <assert.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_SCHED_MAX_THREADS
#define DRV_SCHED_MAX_THREADS 32
#endif


#ifndef DRV_SCHED_MAX_MARKERS
#define DRV_SCHED_MAX_MARKERS 512
#endif


#ifndef DRV_SCHED_MAX_PENDING_WORK
#define DRV_SCHED_MAX_PENDING_WORK 2048
#endif


#ifndef DRV_SCHED_MAX_BLOCKED_WORK
#define DRV_SCHED_MAX_BLOCKED_WORK 2048
#endif


#ifndef DRV_SCHED_MAX_FIBERS
#define DRV_SCHED_MAX_FIBERS 128
#endif


#ifndef DRV_SCHED_PCHECKS
#define DRV_SCHED_PCHECKS 1
#endif


#ifndef DRV_SCHED_LOGGING
#define DRV_SCHED_LOGGING 1
#endif


/* --------------------------------------------------- Thread Abstractions -- */
/*
 * We use the OS's thread libs so a thin wrapper is used to cleanup the logic
 * in the code below.
 */

#if defined(__linux__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <pthread.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <process.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack( push, 8 )
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType;
  LPCSTR szName;
  DWORD dwThreadID;
  DWORD dwFlags;
} THREADNAME_INFO;
#pragma pack(pop)
#else
#error "Unsupported Platform"
#endif

#if defined(__linux__) || defined(__APPLE__)
typedef pthread_t thread_t;
#elif defined(_WIN32)
typedef HANDLE thread_t;
#endif


/* ---------------------------------------------------- Drive sched Context -- */


struct drv_marker {
        int jobs_enqueued;
        int jobs_pending;
        uint32_t instance;
};


struct drv_work {
        drv_task_fn *func;
        void *arg;
        int tid;
        uint64_t this_maker;
};


struct drv_sched_ctx {
        thread_t* threads[DRV_SCHED_MAX_THREADS];
        void* thread_ids[DRV_SCHED_MAX_THREADS];
        
        void* work[DRV_SCHED_MAX_PENDING_WORK];
        
        struct drv_marker markers[DRV_SCHED_MAX_MARKERS];
        uint32_t marker_instance;
        
        void* blocked[DRV_SCHED_MAX_BLOCKED_WORK];
        void* fibers[DRV_SCHED_MAX_FIBERS];


        drv_log_fn log_fn;
};


/* -------------------------------------------------------------- Lifetime -- */


void
drv_fiber_proc(
        void *arg)
{
        (void)arg;
}


void
drv_thread_proc(
        void *arg)
{
        (void)arg;
}


drv_sched_result
drv_sched_ctx_create(
        const struct drv_sched_ctx_create_desc *desc,
        struct drv_sched_ctx **out)
{
        /* pedantic checks */
        if(DRV_SCHED_PCHECKS && !desc) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }

        if(DRV_SCHED_PCHECKS && !out) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }

        if(DRV_SCHED_PCHECKS && !desc->sched_alloc) {
                assert(!"DRV_SCHED_RESULT_INVALID_DESC");
                return DRV_SCHED_RESULT_INVALID_DESC;
        }

        /* create a new ctx */
        unsigned ctx_bytes = sizeof(struct drv_sched_ctx);
        void *alloc = desc->sched_alloc(ctx_bytes);
        memset(alloc, 0, ctx_bytes);

        if(!alloc) {
                assert(!"DRV_SCHED_RESULT_FAIL");
                return DRV_SCHED_RESULT_FAIL;
        }

        struct drv_sched_ctx *new_ctx = alloc;

        /* create resources */
        int i;

        /* mutex */

        /* create threads */
        int th_count = 2;
        
        for(i = 1; i < th_count; ++i) {
                #if defined(__linux__) || defined(__APPLE__)

                #elif defined(_WIN32)
                new_ctx->threads[i] = (HANDLE)(_beginthreadex(
                        NULL,
                        524288,
                        (_beginthreadex_proc_type)drv_thread_proc,
                        NULL, /* arg */
                        0,
                        NULL)
                );
                #endif
        }

        /* fibers */
        int fi_count = 1;

        for(i = 0; i < fi_count; ++i) {
                /* create fibers */
        }

        /* setup ctx */

        /* output */
        /* only assign `out` when setup is complete */
        *out = new_ctx;

        return DRV_SCHED_RESULT_OK;
}


drv_sched_result
drv_sched_ctx_destroy(
        struct drv_sched_ctx_destroy_desc *desc)
{
        if(DRV_SCHED_PCHECKS && !desc) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        if(DRV_SCHED_PCHECKS && !desc->ctx_to_destroy) {
                assert(!"DRV_SCHED_RESULT_INVALID_DESC");
                return DRV_SCHED_RESULT_INVALID_DESC;
        }
        
        /* free if we have been asked to */
        if(desc->sched_free) {
                desc->sched_free(*desc->ctx_to_destroy);
        }
        
        *desc->ctx_to_destroy = 0;
        
        return DRV_SCHED_RESULT_OK;
}


drv_sched_result
drv_sched_ctx_join(
        struct drv_sched_ctx *ctx)
{
        return DRV_SCHED_RESULT_FAIL;
}


/* --------------------------------------------------------------- Enqueue -- */


drv_sched_result
drv_sched_enqueue(
        struct drv_sched_ctx *ctx,
        const struct drv_sched_enqueue_desc *desc,
        uint64_t *out_batch_mk)
{
        if(DRV_SCHED_PCHECKS && !ctx) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        if(DRV_SCHED_PCHECKS && !desc) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        if(DRV_SCHED_PCHECKS && !desc->work_count) {
                assert(!"DRV_SCHED_RESULT_INVALID_DESC");
                return DRV_SCHED_RESULT_INVALID_DESC;
        }
        
        if(DRV_SCHED_PCHECKS && !desc->work) {
                assert(!"DRV_SCHED_RESULT_INVALID_DESC");
                return DRV_SCHED_RESULT_INVALID_DESC;
        }
        
        int i;
        uint64_t index = DRV_SCHED_MAX_MARKERS;
        uint32_t instance = ctx->marker_instance + 1;
        
        /* find a new batch id for this */
        while(index == DRV_SCHED_MAX_MARKERS) {
                for(i = 0; i < DRV_SCHED_MAX_MARKERS; ++i) {
                        if(ctx->markers[i].instance == 0) {
                                index = i;
                                break;
                        }
                }
        }
        
        /* setup marker */
        ctx->markers[i].instance = instance;
        ctx->markers[i].jobs_pending = desc->work_count;
        ctx->markers[i].jobs_enqueued = desc->work_count;
        
        uint64_t mk = (index << 32) | instance;
        
        if(*out_batch_mk) {
                *out_batch_mk = mk;
        }
        
        /* for each job enqueue */
        for(i = 0; i < desc->work_count; ++i) {
                /* insert work into the task queue */
        }
        
        return DRV_SCHED_RESULT_FAIL;
}


drv_sched_result
drv_sched_wait(
        struct drv_sched_ctx *ctx,
        uint64_t wait_mk)
{
        return DRV_SCHED_RESULT_FAIL;
}


/* --------------------------------------------------------------- Details -- */


drv_sched_result
drv_sched_detail_get(
        struct drv_sched_ctx *ctx,
        drv_detail detail,
        int *out)
{
        return DRV_SCHED_RESULT_FAIL;
}


/* -------------------------------------------------------------- Debuging -- */


drv_sched_result
drv_sched_profile_data_get(
        struct drv_sched_ctx *ctx,
        struct drv_sched_profile_data *out_data,
        int *out_count)
{
        return DRV_SCHED_RESULT_FAIL;
}


/* ---------------------------------------------------------------- Config -- */


#undef DRV_SCHED_MAX_THREADS
#undef DRV_SCHED_MAX_MARKERS
#undef DRV_SCHED_MAX_PENDING_WORK
#undef DRV_SCHED_MAX_BLOCKED_WORK
#undef DRV_SCHED_MAX_FIBERS
#undef DRV_SCHED_PCHECKS
#undef DRV_SCHED_LOGGING
