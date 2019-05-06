#include <drive/sched.h>
#include <boost_context/fcontext.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


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


#ifndef DRV_PANIC_LOGGING
#define DRV_PANIC_LOGGING 1
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
#include <sys/sysinfo.h>
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
typedef struct tagTHREADNAME_INFO {
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
typedef pthread_mutex_t mutex_t;
typedef pthread_id_t thread_id_t;
#elif defined(_WIN32)
typedef uintptr_t thread_t;
typedef CRITICAL_SECTION mutex_t;
typedef DWORD thread_id_t;
#endif


struct drv_signal {
        #if defined( _WIN32 )
        CRITICAL_SECTION mutex;
        CONDITION_VARIABLE condition;
        int value;
        #elif defined(__linux__) || defined(__APPLE__)
        pthread_mutex_t mutex;
        pthread_cond_t condition;
        int value;
        #endif
};


void
drv_mutex_create(mutex_t *m)
{
        #if defined(__linux__) || (__APPLE__)
        pthread_mutex_init(m, NULL);
        #elif defined(_WIN32)
        CRITICAL_SECTION *mut = (CRITICAL_SECTION*)m;
        InitializeCriticalSectionAndSpinCount(mut, 32);
        #endif
}

void
drv_mutex_destroy(mutex_t *m)
{
        #if defined(__linux__) || defined(__APPLE__)
        pthread_mutex_destroy((pthread_mutex_t*) m);
        #elif defined(_WIN32)
        DeleteCriticalSection((CRITICAL_SECTION*)m);
        #endif
}


void
drv_mutex_lock(mutex_t *m)
{
        #if defined(__linux__) || defined(__APPLE__)
        pthread_mutex_lock((pthread_mutex_t*)m);
        #elif defined(_WIN32)
        CRITICAL_SECTION *mut = (CRITICAL_SECTION*)m;
        EnterCriticalSection(mut);
        #endif
}


void
drv_mutex_unlock(mutex_t *m)
{
        #if defined(__linux__) || defined(__APPLE__)
        pthread_mutex_unlock((pthread_mutex_t*)m);
        #elif defined(_WIN32)
        LeaveCriticalSection((CRITICAL_SECTION*)m);
        #endif
}


int
drv_physical_core_count()
{
        int cpu_count = 0;

        #if defined(_WIN32)
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        cpu_count = sysinfo.dwNumberOfProcessors; /* need phy cores */
        #elif defined(__linux__)
        cpu_count = get_nprocs();
        #elif defined(__APPLE__)
        size_t count_len = sizeof(cpu_count);
        sysctlbyname("hw.physicalcpu", &cpu_count, &count_len, NULL, 0);
        #endif
        
        return cpu_count;
}


int
drv_logical_core_count()
{
        int cpu_count = 0;

        #ifdef _WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        cpu_count = sysinfo.dwNumberOfProcessors;
        #elif defined(__linux__)
        #error "no impl"
        #elif defined(__APPLE__)
        size_t count_len = sizeof(cpu_count);
        sysctlbyname("hw.logicalcpu", &cpu_count, &count_len, NULL, 0);
        #endif
        
        return cpu_count;
}


/* ---------------------------------------------------- Drive sched Context -- */


struct drv_marker {
        int jobs_enqueued;
        int jobs_pending;
        uint32_t instance;
};


struct drv_work {
        drv_task_fn func;
        void *arg;
        int tid;
        uint64_t marker;
};


struct drv_fiber {
        void *stack;
        size_t sys_page_size;
        size_t stack_size;
        fcontext_t context;
        void *arg;

        struct drv_work work_item;
};


struct drv_thread_arg {
        thread_id_t thread_id;
        char name[64];
        fcontext_t home_context;
        struct drv_fiber *work_fiber;
        struct drv_sched_ctx *ctx;
};


struct drv_sched_ctx {
        mutex_t mut;

        thread_id_t main_thread;

        thread_t threads[DRV_SCHED_MAX_THREADS];
        struct drv_thread_arg thread_args[DRV_SCHED_MAX_THREADS];
        void* thread_ids[DRV_SCHED_MAX_THREADS];
        int thread_count;
        
        struct drv_work work[DRV_SCHED_MAX_PENDING_WORK];
        int work_count;
        
        struct drv_marker markers[DRV_SCHED_MAX_MARKERS];
        int marker_count;
        uint32_t marker_instance;
        
        void* blocked[DRV_SCHED_MAX_BLOCKED_WORK];
        int blocked_count;
        
        struct drv_fiber *free_fibers[DRV_SCHED_MAX_FIBERS];
        int free_fiber_count;
        
        struct drv_fiber fibers[DRV_SCHED_MAX_FIBERS];
        
        int running;

        drv_sched_log_fn log_fn;
};


void
drv_dummy_log(const char *msg) {
        (void)msg;
}


/* -------------------------------------------------------------- Lifetime -- */


void
drv_fiber_proc(
        void *arg)
{
        int i;
        struct drv_sched_ctx *ctx = (struct drv_thread_arg*)arg;

        if(DRV_PANIC_LOGGING) {
                printf("DRV Fiber Started\n");
        }
        
        for(;;) {
                if(DRV_PANIC_LOGGING) {
                        printf("DRV Fiber Setup\n");
                }

                /* reaquire thread arg */
                thread_id_t th_id = GetCurrentThreadId();

                struct drv_thread_arg *th_arg = 0;

                for(i = 0; i < ctx->thread_count; ++i) {
                        if (ctx->thread_args[i].thread_id == th_id) {
                                th_arg = &ctx->thread_args[i];
                                break;
                        }
                }
                
                struct drv_fiber *fi = th_arg->work_fiber;

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Fiber %p Dispatching\n", fi);
                }
        
                /* run the task */
                drv_task_fn task_fn = fi->work_item.func;
                void *task_arg      = fi->work_item.arg;
                
                task_fn(th_arg->ctx, task_arg);

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Fiber %p Job done\n", fi);
                }
                
                /* update the marker */
                drv_mutex_lock(&ctx->mut);
                
                uint64_t marker   = fi->work_item.marker;
                uint32_t instance = (uint32_t)(marker & 0xFFFFFFFF);
                uint32_t index    = (uint32_t)((marker >> 32) & 0xFFFFFFFF);
                
                struct drv_marker *mk = &ctx->markers[index];
                assert(mk->instance == instance);
                
                mk->jobs_pending -= 1;

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Marker %d counter dec %d / %d\n",
                                instance,
                                mk->jobs_pending,
                                mk->jobs_enqueued);
                }
                
                if(mk->jobs_pending == 0) {
                        mk->instance = 0;
                        mk->jobs_enqueued = 0;
                        mk->jobs_pending = 0;
                }

                drv_mutex_unlock(&ctx->mut);
                
                /* switch back to thread proc */
                fcontext_t *this_fi = &fi->context;
                fcontext_t that_fi = th_arg->home_context;

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Fiber %p Jump back to thread proc\n", fi);
                }

                jump_fcontext(this_fi, that_fi, NULL);
        }
}


void*
drv_thread_proc(
        void *arg)
{
        struct drv_thread_arg *th_arg = (struct drv_thread_arg*)arg;
        th_arg->thread_id = GetCurrentThreadId();
        th_arg->home_context = NULL;
        struct drv_sched_ctx *ctx = th_arg->ctx;

        /* debug info */
        int dispatched = 0;
        int finished = 0;

        /* we have todo it like this because macOS's pthread doesn't take
         a thread id */
        if(strlen(th_arg->name)) {
                #if defined(__linux__)
                int success = pthread_setname_np(pthread_self(), th_arg->name);
                assert(success == 0);
                #elif defined(__APPLE__)
                int success = pthread_setname_np(th_arg->name);
                assert(success == 0);
                #elif defined(_WIN32)
                THREADNAME_INFO in;
                in.dwType = 0x1000;
                in.szName = th_arg->name;
                in.dwThreadID = GetCurrentThreadId();
                in.dwFlags = 0;

                __try {
                        long si = sizeof(in) / sizeof(ULONG_PTR);
                        RaiseException(MS_VC_EXCEPTION, 0, si, (ULONG_PTR*)&in);
                }
                __except(EXCEPTION_EXECUTE_HANDLER) {}
                #endif
        }
                
        /* search for a job */
        while(ctx->running) {
                /* allow main thread to quit if work is done */
                if(th_arg->thread_id == ctx->thread_args[0].thread_id) {
                        if(ctx->free_fiber_count == DRV_SCHED_MAX_FIBERS &&
                           ctx->work_count == 0 &&
                           ctx->blocked_count == 0)
                        {
                                break;
                        }
                }
                
                /* look at pending jobs */
                drv_mutex_lock(&ctx->mut);
                
                int i;
                struct drv_work *work = 0;
                int wk_idx = 0;
                struct drv_fiber *fi = 0;
                
                for(i = 0; i < ctx->work_count; ++i) {
                        struct drv_work *w = &ctx->work[i];
                        
                        if(w->tid == -1 || w->tid == th_arg->thread_id) {
                                work = w;
                                wk_idx = i;
                                break;
                        }
                }
                
                /* look for fiber */
                if(work) {
                        assert(ctx->free_fiber_count);
                        
                        if(ctx->free_fiber_count) {
                                int fi_idx = ctx->free_fiber_count - 1;
                                fi = ctx->free_fibers[fi_idx];

                                ctx->free_fiber_count -= 1;
                        }
                        
                        if(!fi) {
                            assert(!"No fiber found.");
                        }
                }
                
                if(!work || !fi) {
                        drv_mutex_unlock(&ctx->mut);
                        /* sleep */
                        continue;
                }
                
                /* remove work from queue */
                fi->work_item = *work;
                th_arg->work_fiber = fi;
                
                void *dst = &ctx->work[wk_idx];
                void *src = &ctx->work[wk_idx + 1];
                
                int size = sizeof(ctx->work) / sizeof(ctx->work[0]);
                int count = size - wk_idx - 1;
                int bytes = sizeof(ctx->work[0]) * count;
                
                memmove(dst, src, bytes);
                ctx->work_count -= 1;
                
                drv_mutex_unlock(&ctx->mut);
                
                /* jump into fiber proc */
                fcontext_t *this_fi = &th_arg->home_context;
                fcontext_t that_fi = fi->context;

                dispatched += 1;

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Thread %d Jump to Fiber %p\n",
                                th_arg->thread_id,
                                fi);
                }

                jump_fcontext(this_fi, that_fi, th_arg->ctx);
                
                /* jumped back to thread proc */
                finished += 1;

                /* add fiber back to free queue */
                /* reaquire fiber data as it might have changed in jumps */
                fi = th_arg->work_fiber;
                drv_mutex_lock(&ctx->mut);

                int last_idx = ctx->free_fiber_count;
                ctx->free_fibers[last_idx] = fi;
                ctx->free_fiber_count += 1;

                drv_mutex_unlock(&ctx->mut);
        }
        
        return 0;
}


int
drv_sched_setup_threads(
        const struct drv_sched_ctx_create_desc *desc,
        struct drv_sched_ctx *ctx)
{
        int i;
        
        /* mutex */
        drv_mutex_create(&ctx->mut);
        drv_mutex_lock(&ctx->mut);
        
        /* thread count */
        int th_count = desc->thread_count;
        
        if(desc->thread_count <= 0) {
                th_count = drv_logical_core_count() - 1;
        }
        
        if(th_count <= 0) {
                th_count = 2;
        }
        
        if(DRV_SCHED_LOGGING) {
                char buf[256];
                snprintf(buf, sizeof(buf), "Create %d threads", th_count);
                ctx->log_fn(&buf[0]);
        }
        
        /* setup thread args */
        for(i = 0; i < th_count; ++i) {
                ctx->thread_args[i].ctx = ctx;
                
                /* copy thread name */
                char *dst = ctx->thread_args[i].name;
                const char *src = desc->thread_name;
                size_t b_count = strlen(ctx->thread_args[i].name);
                strncat(dst, src, b_count - 1);
        }


        /* thread id for main thread */
        ctx->main_thread = (thread_id_t)GetCurrentThreadId();
        
        /* create threads */
        for(i = 1; i < th_count; ++i) {
                void *arg = (void*)&ctx->thread_args[i];
        
                #if defined(__linux__) || defined(__APPLE__)
                pthread_t th;
                
                int success = pthread_create(
                        &th,
                        NULL,
                        drv_thread_proc,
                        arg);
                
                assert(success == 0 && "Failed to create thread");
                #elif defined(_WIN32)
                uintptr_t th = _beginthreadex(
                        NULL,
                        524288,
                        (_beginthreadex_proc_type)drv_thread_proc,
                        arg,
                        0,
                        NULL);
                
                assert(th && "Failed to create thread");
                #endif
                
                if(!th) {
                        drv_mutex_unlock(&ctx->mut);
                        return 0;
                }
                
                ctx->threads[i] = (thread_t)th;
        }

        ctx->thread_count = th_count;
        
        /* thread affinity */
        if(desc->thread_pin == 1) {
                if(DRV_SCHED_LOGGING) {
                        char buf[128];
                        snprintf(buf, sizeof(buf), "Lock threads to cores");
                        ctx->log_fn(&buf[0]);
                }
                
                for(i = 0; i < th_count; ++i) {
                        #if defined(__linux__)
                        #warning "platform not supported"
                        #elif defined(_WIN32)
                        HANDLE th = (HANDLE)ctx->threads[i];
                        int core_count = drv_physical_core_count();
                        SetThreadIdealProcessor(th, i % core_count);
                        #else
                        #warning "platform not supported"
                        #endif
                }
        }
        
        drv_mutex_unlock(&ctx->mut);
        return 1;
}

int
drv_sched_setup_fibers(
        const struct drv_sched_ctx_create_desc *desc,
        struct drv_sched_ctx *ctx)
{
        int i;
        int stack_size = 524288;
        int bytes = stack_size + stack_size + stack_size;
        
        /* create fibers */
        for(i = 0; i < DRV_SCHED_MAX_FIBERS; ++i) {
                struct drv_fiber *fi = &ctx->fibers[i];
        
                fi->stack = desc->sched_alloc(bytes);
                
                if(!fi->stack) {
                        assert(!"Failed to allocate space for fiber");
                        return 0;
                }
                
                memset(fi->stack, 0, bytes);
                
                void *start = ((char*)fi->stack) + stack_size;
                fi->context = make_fcontext(start, stack_size, drv_fiber_proc);
        }
        
        /* create fiber free list */
        for(i = 0; i < DRV_SCHED_MAX_FIBERS; ++i) {
                ctx->free_fibers[i] = &ctx->fibers[i];
        }
        
        ctx->free_fiber_count = DRV_SCHED_MAX_FIBERS;
        
        return 1;
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
        
        /* logging */
        if(DRV_SCHED_LOGGING && desc->sched_log) {
                new_ctx->log_fn = desc->sched_log;
        }
        else if(DRV_SCHED_LOGGING) {
                new_ctx->log_fn = drv_dummy_log;
        }
        
        new_ctx->running = 1;

        /* create resources */
        int success = 0;

        success = drv_sched_setup_threads(desc, new_ctx);
        if(!success) {
                assert(!"DRV_SCHED_RESULT_FAIL");
                goto DRV_FAILED_SETUP;
        }
        
        /* fibers */
        success = drv_sched_setup_fibers(desc, new_ctx);
        if(!success) {
                assert(!"DRV_SCHED_RESULT_FAIL");
                goto DRV_FAILED_SETUP;
        }

        /* setup ctx */

        /* output */
        /* only assign `out` when setup is complete */
        *out = new_ctx;

        return DRV_SCHED_RESULT_OK;
        
        /* failed to create context backing out */
        DRV_FAILED_SETUP: {
                drv_sched_result success = DRV_SCHED_RESULT_OK;
                
                struct drv_sched_ctx_destroy_desc destroy_desc = {0};
                destroy_desc.ctx_to_destroy = &new_ctx;
                destroy_desc.sched_free = 0; /* not our job to free */
                
                success = drv_sched_ctx_destroy(&destroy_desc);
                
                assert(success == DRV_SCHED_RESULT_OK && "DRV_SCHED_RESULT_OK");
                
                return DRV_SCHED_RESULT_FAIL;
        }
}


drv_sched_result
drv_sched_ctx_destroy(
        struct drv_sched_ctx_destroy_desc *desc)
{
        /* pedantic checks */
        if(DRV_SCHED_PCHECKS && !desc) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        if(DRV_SCHED_PCHECKS && !desc->ctx_to_destroy) {
                assert(!"DRV_SCHED_RESULT_INVALID_DESC");
                return DRV_SCHED_RESULT_INVALID_DESC;
        }
        
        struct drv_sched_ctx *ctx = *desc->ctx_to_destroy;
        ctx->running = 0;

        int i;
        int th_count = ctx->thread_count;
                
        /* destroy threads */
        for(i = 1; i < th_count; ++i) {
                if(!ctx->threads[i]) {
                        continue;
                }
        
                #if defined(__linux__) || defined(__APPLE__)
                pthread_t th = (pthread_t)ctx->threads[i];
                pthread_join(th, 0);
                #elif defined(_WIN32)
                uintptr_t th = (HANDLE)ctx->threads[i];
                WaitForSingleObject(th, INFINITE);
                CloseHandle(th);
                #endif
                
                ctx->threads[i] = 0;
        }
        
        drv_mutex_destroy(&ctx->mut);
        
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
        /* pedantic checks */
        if(DRV_SCHED_PCHECKS && !ctx) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }

        if(DRV_SCHED_PCHECKS) {
                thread_id_t this_id = (thread_id_t)GetCurrentThreadId();

                if (this_id != ctx->main_thread) {
                        assert(!"DRV_SCHED_RESULT_BAD_CALL");
                        return DRV_SCHED_RESULT_BAD_CALL;
                }
        }

        drv_thread_proc(&ctx->thread_args[0]);

        return DRV_SCHED_RESULT_OK;
}


/* --------------------------------------------------------------- Enqueue -- */


drv_sched_result
drv_sched_enqueue(
        struct drv_sched_ctx *ctx,
        const struct drv_sched_enqueue_desc *desc,
        uint64_t *out_batch_mk)
{
        /* pedantic checks */
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
        
        drv_mutex_lock(&ctx->mut);
        
        if(DRV_SCHED_PCHECKS) {
                int needed = desc->work_count + ctx->work_count;
                if(needed > DRV_SCHED_MAX_PENDING_WORK) {
                        /* increase max work items */
                        assert(!"DRV_SCHED_RESULT_OUT_OF_RESOURCES");
                        
                        drv_mutex_unlock(&ctx->mut);
                        return DRV_SCHED_RESULT_OUT_OF_RESOURCES;
                }
        }
        
        /* new marker */
        int i;
        uint64_t index    = DRV_SCHED_MAX_MARKERS;
        uint32_t instance = ++ctx->marker_instance;
        
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
        ctx->markers[i].instance      = instance;
        ctx->markers[i].jobs_pending  = desc->work_count;
        ctx->markers[i].jobs_enqueued = desc->work_count;
        
        /* marker is packed index and instance */
        uint64_t mk = (index << 32) | instance;
        
        if(out_batch_mk) {
                *out_batch_mk = mk;
        }
        
        /* insert work into the task queue */
        int in_idx = ctx->work_count;
        for(i = 0; i < desc->work_count; ++i, ++in_idx) {
                drv_task_fn fn = desc->work[i].func;
                void *arg = desc->work[i].arg;
                int tid = desc->work[i].tid_lock ? 1 : -1;
                
                /* insert work */
                ctx->work[in_idx].marker = mk;
                ctx->work[in_idx].arg    = arg;
                ctx->work[in_idx].func   = fn;
                ctx->work[in_idx].tid    = tid;
        }
        
        ctx->work_count += desc->work_count;
        
        drv_mutex_unlock(&ctx->mut);
        
        return DRV_SCHED_RESULT_OK;
}


drv_sched_result
drv_sched_wait(
        struct drv_sched_ctx *ctx,
        uint64_t wait_mk)
{
        /* pedantic checks */
        if(DRV_SCHED_PCHECKS && !ctx) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        if(DRV_SCHED_PCHECKS && !wait_mk) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }

        int i;

        /* find thread */
        thread_id_t self_id = GetCurrentThreadId();
        int tls = -1;

        for(i = 0; i < ctx->thread_count; ++i) {
                if(ctx->thread_args[i].thread_id == tls) {
                        tls = i;
                        break;
                };
        };
        
        /* block current task switch fiber back to thread_proc */
        if(tls != -1) {
                /* push fiber to blocked */
                drv_mutex_lock(&ctx->mut);

                drv_mutex_unlock(&ctx->mut);
        }

        /* thread not in pool so spin until marker is done */
        else {
                for(;;) {
                        int found = 1;
        
                        drv_mutex_lock(&ctx->mut);

                        uint32_t instance = (uint32_t)(wait_mk & 0xFFFFFFFF);
                        uint32_t idx = (uint32_t)((wait_mk >> 32) & 0xFFFFFFFF);

                        if(ctx->markers[idx].instance != instance) {
                                found = 0;
                        }

                        drv_mutex_unlock(&ctx->mut);
                
                        if(found) {
                                Sleep(10);
                        } else {
                                break;
                        }
                }
        }

        return DRV_SCHED_RESULT_OK;
}


/* --------------------------------------------------------------- Details -- */


drv_sched_result
drv_sched_ctx_detail_get(
        struct drv_sched_ctx *ctx,
        drv_ctx_detail detail,
        int *out)
{
        /* pedantic checks */
        if(DRV_SCHED_PCHECKS && !ctx) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        if(DRV_SCHED_PCHECKS && !out) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        /* details */
        if(detail == DRV_SCHED_CTX_DETAIL_MAX_JOBS) {
                *out = DRV_SCHED_MAX_PENDING_WORK;
                return DRV_SCHED_RESULT_OK;
        }
        else if(detail == DRV_SCHED_CTX_DETAIL_MAX_MARKERS) {
                *out = DRV_SCHED_MAX_MARKERS;
                return DRV_SCHED_RESULT_OK;
        }
        else if(detail == DRV_SCHED_CTX_DETAIL_MAX_FIBERS) {
                *out = DRV_SCHED_MAX_FIBERS;
                return DRV_SCHED_RESULT_OK;
        }
        else if(detail == DRV_SCHED_CTX_DETAIL_THREAD_COUNT) {
                *out = ctx->thread_count;
                return DRV_SCHED_RESULT_OK;
        }
        else if(detail == DRV_SCHED_CTX_DETAIL_PEDANTIC_CHECKS) {
                *out = !!DRV_SCHED_PCHECKS;
                return DRV_SCHED_RESULT_OK;
        }
        
        assert(!"Unhandled detail");
        return DRV_SCHED_RESULT_FAIL;
}


drv_sched_result
drv_sched_sys_detail_get(
        drv_ctx_detail detail,
        int *out)
{
        if(DRV_SCHED_PCHECKS && !out) {
                assert(!"DRV_SCHED_RESULT_BAD_PARAM");
                return DRV_SCHED_RESULT_BAD_PARAM;
        }
        
        if(detail == DRV_SCHED_SYS_DETAIL_PHYSICAL_CORE_COUNT) {
                *out = drv_physical_core_count();
                return DRV_SCHED_RESULT_OK;
        }
        else if(detail == DRV_SCHED_SYS_DETAIL_LOGICAL_CORE_COUNT) {
                *out = drv_logical_core_count();
                return DRV_SCHED_RESULT_OK;
        }
        
        assert(!"Unhandled detail");
        return DRV_SCHED_RESULT_FAIL;
}


/* -------------------------------------------------------------- Debuging -- */


drv_sched_result
drv_sched_profile_data_get(
        struct drv_sched_ctx *ctx,
        struct drv_sched_profile_data *out_data,
        int *out_count)
{
        (void)ctx;
        (void)out_data;
        (void)out_count;

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
