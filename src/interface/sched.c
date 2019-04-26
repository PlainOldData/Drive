#include <drive/sched.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_SHED_MAX_THREADS
#define DRV_SHED_MAX_THREADS 32
#endif


#ifndef DRV_SHED_MAX_MARKERS
#define DRV_SHED_MAX_MARKERS 512
#endif


#ifndef DRV_SHED_MAX_PENDING_WORK
#define DRV_SHED_MAX_PENDING_WORK 2048
#endif


#ifndef DRV_SHED_MAX_BLOCKED_WORK
#define DRV_SHED_MAX_BLOCKED_WORK 2048
#endif


#ifndef DRV_SHED_MAX_FIBERS
#define DRV_SHED_MAX_FIBERS 128
#endif


#ifndef DRV_SHED_PCHECKS
#define DRV_SHED_PCHECKS 1
#endif


#ifndef DRV_SHED_LOGGING
#define DRV_SHED_LOGGING 1
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


/* ---------------------------------------------------- Drive Shed Context -- */


struct drv_shed_ctx {
        thread_t* threads[DRV_SHED_MAX_THREADS];
        void* thread_ids[DRV_SHED_MAX_THREADS];
        void* markers[DRV_SHED_MAX_MARKERS];
        void* work[DRV_SHED_MAX_PENDING_WORK];
        void* blocked[DRV_SHED_MAX_BLOCKED_WORK];
        void* fibers[DRV_SHED_MAX_FIBERS];

        drv_log_fn log_fn;
};


/* -------------------------------------------------------------- Lifetime -- */


void
drv_fiber_proc(
        void *arg)
{

}


void
drv_thread_proc(
        void *arg)
{

}


drv_shed_result
drv_shed_ctx_create(
        const struct drv_shed_ctx_create_desc *desc,
        struct drv_shed_ctx **out)
{
        /* pedantic checks */
        if(DRV_SHED_PCHECKS && !desc) {
                assert(!"DRV_SHED_RESULT_BAD_PARAM");
                return DRV_SHED_RESULT_BAD_PARAM;
        }

        if(DRV_SHED_PCHECKS && !out) {
                assert(!"DRV_SHED_RESULT_BAD_PARAM");
                return DRV_SHED_RESULT_BAD_PARAM;
        }

        if(DRV_SHED_PCHECKS && !desc->shed_alloc) {
                assert(!"DRV_SHED_RESULT_INVALID_DESC");
                return DRV_SHED_RESULT_INVALID_DESC;       
        }

        /* create a new ctx */
        unsigned ctx_bytes = sizeof(struct drv_shed_ctx);
        void *alloc = desc->shed_alloc(ctx_bytes);

        if(!alloc) {
                assert(!"DRV_SHED_RESULT_FAIL");
                return DRV_SHED_RESULT_FAIL;
        }

        struct drv_shed_ctx *new_ctx = alloc;

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

        return DRV_SHED_RESULT_OK;
}


drv_shed_result
drv_shed_ctx_destroy(
        struct drv_shed_ctx **destroy)
{
        return DRV_SHED_RESULT_FAIL;
}


drv_shed_result
drv_shed_ctx_join(
        struct drv_shed_ctx *ctx)
{
        return DRV_SHED_RESULT_FAIL;
}


/* --------------------------------------------------------------- Enqueue -- */


drv_shed_result
drv_shed_enqueue(
        struct drv_shed_ctx *ctx,
        const struct drv_shed_enqueue_desc *desc,
        uint64_t *batch_mk)
{
        return DRV_SHED_RESULT_FAIL;
}


drv_shed_result
drv_shed_wait(
        struct drv_shed_ctx *ctx,
        uint64_t wait_mk)
{
        return DRV_SHED_RESULT_FAIL;
}


/* --------------------------------------------------------------- Details -- */


drv_shed_result
drv_shed_detail_get(
        struct drv_shed_ctx *ctx,
        drv_detail detail,
        int *out)
{
        return DRV_SHED_RESULT_FAIL;
}


/* -------------------------------------------------------------- Debuging -- */


drv_shed_result
drv_shed_profile_data_get(
        struct drv_shed_ctx *ctx,
        struct drv_shed_profile_data *out_data,
        int *out_count)
{
        return DRV_SHED_RESULT_FAIL;
}


/* ---------------------------------------------------------------- Config -- */


#undef DRV_SHED_MAX_THREADS
#undef DRV_SHED_MAX_MARKERS
#undef DRV_SHED_MAX_PENDING_WORK
#undef DRV_SHED_MAX_BLOCKED_WORK
#undef DRV_SHED_MAX_FIBERS
#undef DRV_SHED_PCHECKS
#undef DRV_SHED_LOGGING
