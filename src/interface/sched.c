#include <drive/sched.h>
#include <boost_context/fcontext.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


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
#define DRV_PANIC_LOGGING 0
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
static const DWORD MS_VC_EXCEPTION = 0x406D1388;
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
typedef uint64_t thread_id_t;
#elif defined(_WIN32)
typedef uintptr_t thread_t;
typedef CRITICAL_SECTION mutex_t;
typedef DWORD thread_id_t;
#endif


struct drv_signal {
        #if defined(_WIN32)
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
drv_signal_create(struct drv_signal *signal) {
        #if defined(_WIN32)
        InitializeCriticalSectionAndSpinCount(&signal->mutex, 32);
        InitializeConditionVariable(&signal->condition);
        signal->value = 0;
        #elif defined(__linux__) || defined(__APPLE__)
        pthread_mutex_init(&signal->mutex, NULL);
        pthread_cond_init(&signal->condition, NULL);
        signal->value = 0;
        #endif
}

void
drv_signal_destroy(struct drv_signal *signal) {
        #if defined(_WIN32)
        DeleteCriticalSection(&signal->mutex);
        #elif defined(__linux__) || defined(__APPLE__)
        pthread_mutex_destroy(&signal->mutex);
        pthread_cond_destroy(&signal->condition);
        #endif
}


void
drv_signal_raise(struct drv_signal *signal) {
        #if defined(_WIN32)
        EnterCriticalSection(&signal->mutex);
        signal->value = 1;
        LeaveCriticalSection(&signal->mutex);
        WakeConditionVariable(&signal->condition);
        #elif defined(__linux__) || defined(__APPLE__)
        pthread_mutex_lock(&signal->mutex);
        signal->value = 1;
        pthread_mutex_unlock(&signal->mutex);
        pthread_cond_signal(&signal->condition);
        #endif
}


int
drv_signal_wait(struct drv_signal *signal, int timeout_ms) {
        #if defined( _WIN32 )
        int timed_out = 0;
        EnterCriticalSection(&signal->mutex);
        while(signal->value == 0) {
                int res = SleepConditionVariableCS(
                        &signal->condition,
                        &signal->mutex,
                        timeout_ms < 0 ? INFINITE : timeout_ms);
                
                if (!res && GetLastError() == ERROR_TIMEOUT) {
                        timed_out = 1; break;
                }
        }
        if(!timed_out) {
                signal->value = 0;
        }
        LeaveCriticalSection(&signal->mutex);
        return !timed_out;
        #elif defined(__linux__) || defined(__APPLE__)

        struct timespec ts;
        if(timeout_ms >= 0) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                ts.tv_sec = time(NULL) + timeout_ms / 1000;
                ts.tv_nsec = tv.tv_usec*1000+1000*1000*(timeout_ms%1000);
                ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
                ts.tv_nsec %= (1000 * 1000 * 1000);
        }

        int timed_out = 0;
        pthread_mutex_lock(&signal->mutex);
        while(signal->value == 0) {
                if(timeout_ms < 0) {
                        pthread_cond_wait(&signal->condition, &signal->mutex);
                }
                else if(pthread_cond_timedwait(
                        &signal->condition,
                        &signal->mutex, &ts) == ETIMEDOUT)
                {
                        timed_out = 1;
                        break;
                }
        }
        if(!timed_out) {
                signal->value = 0;
        }
        pthread_mutex_unlock(&signal->mutex);
        return !timed_out;
        #endif
}


void
drv_mutex_create(mutex_t *m) {
        #if defined(__linux__) || (__APPLE__)
        pthread_mutex_init(m, NULL);
        #elif defined(_WIN32)
        CRITICAL_SECTION *mut = (CRITICAL_SECTION*)m;
        InitializeCriticalSectionAndSpinCount(mut, 32);
        #endif
}

void
drv_mutex_destroy(mutex_t *m) {
        #if defined(__linux__) || defined(__APPLE__)
        pthread_mutex_destroy((pthread_mutex_t*) m);
        #elif defined(_WIN32)
        DeleteCriticalSection((CRITICAL_SECTION*)m);
        #endif
}


void
drv_mutex_lock(mutex_t *m) {
        #if defined(__linux__) || defined(__APPLE__)
        pthread_mutex_lock((pthread_mutex_t*)m);
        #elif defined(_WIN32)
        CRITICAL_SECTION *mut = (CRITICAL_SECTION*)m;
        EnterCriticalSection(mut);
        #endif
}


void
drv_mutex_unlock(mutex_t *m) {
        #if defined(__linux__) || defined(__APPLE__)
        pthread_mutex_unlock((pthread_mutex_t*)m);
        #elif defined(_WIN32)
        LeaveCriticalSection((CRITICAL_SECTION*)m);
        #endif
}


thread_id_t
drv_thread_id_self() {
        thread_id_t th_id;
        #ifdef WIN32
        th_id = GetCurrentThreadId();
        #else
        pthread_threadid_np(NULL, &th_id);
        #endif
        
        return th_id;
}


int
drv_thread_id_equal(thread_id_t a, thread_id_t b) {
        #ifdef WIN32
        return a == b;
        #else
        return pthread_equal(a, b) != 0 ? 1 : 0 ;
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


/* --------------------------------------------------- Drive sched Context -- */


struct drv_marker {
        int jobs_enqueued;
        int jobs_pending;
        uint32_t instance;
};


struct drv_work {
        drv_task_fn func;
        void *arg;
        thread_id_t tid;
        uint64_t marker;
        uint64_t blocked_mk;
};


struct drv_fiber {
        void *stack;
        size_t sys_page_size;
        size_t stack_size;
        fcontext_t context;
        void *arg;

        uint64_t blocked_marker;
        struct drv_work work_item;
};


struct drv_blocked_work {
        uint64_t marker;
        struct drv_fiber *fiber;
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
        struct drv_signal work_signal;
        
        thread_id_t main_thread;

        thread_t threads[DRV_SCHED_MAX_THREADS];
        struct drv_thread_arg thread_args[DRV_SCHED_MAX_THREADS];
        void* thread_ids[DRV_SCHED_MAX_THREADS];
        int thread_count;
        
        struct drv_work work[DRV_SCHED_MAX_PENDING_WORK];
        int work_count;
        
        struct drv_marker markers[DRV_SCHED_MAX_MARKERS];
        uint32_t marker_instance;
        
        struct drv_fiber *blocked[DRV_SCHED_MAX_BLOCKED_WORK];
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
        struct drv_sched_ctx *ctx = (struct drv_sched_ctx*)arg;

        if(DRV_PANIC_LOGGING) {
                printf("DRV Fiber Started\n");
        }
        
        for(;;) {
                if(DRV_PANIC_LOGGING) {
                        printf("DRV Fiber Setup\n");
                }

                /* reaquire thread arg */
                thread_id_t th_id = drv_thread_id_self();

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
                
                /* cleanup */
                drv_mutex_lock(&ctx->mut);
                
                /* reaquire thread arg */
                th_id = drv_thread_id_self();
                th_arg = 0;

                for(i = 0; i < ctx->thread_count; ++i) {
                        if (ctx->thread_args[i].thread_id == th_id) {
                                th_arg = &ctx->thread_args[i];
                                break;
                        }
                }
                
                /* update the marker */
                uint64_t marker   = fi->work_item.marker;
                uint64_t instance = (marker & 0xFFFFFFFF);
                uint64_t index    = ((marker >> 32) & 0xFFFFFFFF);
                
                struct drv_marker *mk = &ctx->markers[index];
                assert(mk->instance == instance);
                
                mk->jobs_pending -= 1;

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Marker %d counter dec %d / %d\n",
                                (int)instance,
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
                fcontext_t *fi_proc = &fi->context;
                fcontext_t th_proc = th_arg->home_context;

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Fiber %p Jump back to thread proc\n",
                                fi);
                }

                jump_fcontext(fi_proc, th_proc, NULL);
        }
}


void*
drv_thread_proc(
        void *arg)
{
        struct drv_thread_arg *th_arg = (struct drv_thread_arg*)arg;
        
        th_arg->thread_id = drv_thread_id_self();
        thread_id_t this_tid = th_arg->thread_id;
        
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
                struct drv_fiber *fiber_work = 0;
        
                /* allow main thread to quit if work is done */
                drv_mutex_lock(&ctx->mut);
                
                thread_id_t main_tid = ctx->thread_args[0].thread_id;
                
                if(drv_thread_id_equal(this_tid, main_tid)) {
                        if(ctx->free_fiber_count == DRV_SCHED_MAX_FIBERS &&
                           ctx->work_count == 0 &&
                           ctx->blocked_count == 0)
                        {
                                drv_mutex_unlock(&ctx->mut);
                                break;
                        }
                }
                drv_mutex_unlock(&ctx->mut);

                int i;
                
                /* look at blocked jobs */
                drv_mutex_lock(&ctx->mut);
                
                int blocked = !!ctx->blocked_count;
                
                if(blocked) {
                        for(i = 0; i < DRV_SCHED_MAX_BLOCKED_WORK; ++i) {
                                struct drv_fiber *bf = ctx->blocked[i];
                        
                                if(bf == 0) {
                                        continue;
                                }
                                
                                thread_id_t tid = bf->work_item.tid;
                                
                                if(tid != 0) {
                                        if(!drv_thread_id_equal(tid, this_tid)) {
                                                continue;
                                        }
                                }
                        
                                uint64_t mk = ctx->blocked[i]->blocked_marker;
                                
                                uint64_t instance = (mk & 0xFFFFFFFF);
                                uint64_t idx = ((mk >> 32) & 0xFFFFFFFF);
                                
                                if(ctx->markers[idx].instance == instance) {
                                        continue;
                                }
                                
                                fiber_work = ctx->blocked[i];
                                fiber_work->blocked_marker = 0;
                                
                                ctx->blocked[i] = 0;
                                ctx->blocked_count -= 1;
                                
                                break;
                        }
                }
        
                drv_mutex_unlock(&ctx->mut);
                
                
                /* look at pending jobs */
                drv_mutex_lock(&ctx->mut);
                
                if(!fiber_work && ctx->work_count) {
                        struct drv_work *work = 0;
                        int wk_idx = 0;
                        
                        for(i = 0; i < ctx->work_count; ++i) {
                                struct drv_work *w = &ctx->work[i];
                                
                                if(w->tid != 0) {
                                        int th = drv_thread_id_equal(
                                                w->tid,
                                                this_tid);
                                        
                                        if(!th) {
                                                continue;
                                        }
                                }
                                
                                uint64_t b_mk = w->blocked_mk;
                                
                                if(b_mk != 0) {
                                        uint64_t instance = (b_mk & 0xFFFFFFFF);
                                        uint64_t idx = ((b_mk >> 32) & 0xFFFFFFFF);
                                        
                                        if(ctx->markers[idx].instance == instance) {
                                                continue;
                                        }
                                }
                                
                                work = w;
                                wk_idx = i;
                                break;
                        }
                        
                        /* look for fiber */
                        if(work) {
                                assert(ctx->free_fiber_count);
                                
                                if(ctx->free_fiber_count) {
                                        int fi_idx = ctx->free_fiber_count - 1;
                                        fiber_work = ctx->free_fibers[fi_idx];

                                        ctx->free_fiber_count -= 1;
                                }
                                
                                if(!fiber_work) {
                                    assert(!"No fiber found.");
                                }
                        } else {
                                drv_mutex_unlock(&ctx->mut);
                                continue;
                        }
                        
                        if(work && !fiber_work) {
                                drv_mutex_unlock(&ctx->mut);
                                assert(0); /* likely gonna die */
                                continue;
                        }
                        
                        /* remove work from queue */
                        fiber_work->work_item = *work;
                        fiber_work->blocked_marker = 0;
                        th_arg->work_fiber = fiber_work;
                        
                        void *dst = &ctx->work[wk_idx];
                        void *src = &ctx->work[wk_idx + 1];
                        
                        int size = sizeof(ctx->work) / sizeof(ctx->work[0]);
                        int count = size - wk_idx - 1;
                        int bytes = sizeof(ctx->work[0]) * count;
                        
                        memmove(dst, src, bytes);
                        ctx->work_count -= 1;
                }
                drv_mutex_unlock(&ctx->mut);
                
                /* jump into work */
                if(fiber_work) {
                        th_arg->work_fiber = fiber_work;
                
                        /* jump into fiber proc */
                        fcontext_t *th_proc = &th_arg->home_context;
                        fcontext_t fi_proc = fiber_work->context;

                        dispatched += 1;

                        if(DRV_PANIC_LOGGING) {
                                printf("DRV Thread %d Jump to Fiber %p\n",
                                        (int)this_tid,
                                        fiber_work);
                        }
                        
                        assert(fiber_work->blocked_marker == 0);
        
                        jump_fcontext(th_proc, fi_proc, th_arg->ctx);
                        
                        /* jumped back to thread proc - cleanup */
                        drv_mutex_lock(&ctx->mut);
                        
                        /* reaquire data as it might have changed in jumps */
                        fiber_work = th_arg->work_fiber;
                        
                        /* if not blocked we can add fiber back to free list */
                        if(!fiber_work->blocked_marker) {
                                finished += 1;

                                /* add fiber back to free queue */
    
                                int last_idx = ctx->free_fiber_count;
                                ctx->free_fibers[last_idx] = fiber_work;
                                ctx->free_fiber_count += 1;
                                
                                th_arg->work_fiber = 0;
                        }
                        else {
                                /* push fiber to blocked queue */
                                int count = ctx->blocked_count;
                                assert(count < DRV_SCHED_MAX_BLOCKED_WORK);
                        
                                for(i = 0; i < DRV_SCHED_MAX_BLOCKED_WORK; ++i){
                                        if(ctx->blocked[i] != 0) {
                                                continue;
                                        }
                                        
                                        ctx->blocked[i] = th_arg->work_fiber;
                                        ctx->blocked_count += 1;
                                        
                                        break;
                                }
                                
                                th_arg->work_fiber = 0;
                        }
                        
                        drv_mutex_unlock(&ctx->mut);
                }
                else {
                        drv_signal_wait(&ctx->work_signal, 1000);
                }
        }
        
        return 0;
}


int
drv_sched_setup_threads(
        const struct drv_sched_ctx_create_desc *desc,
        struct drv_sched_ctx *ctx)
{
        int i;
        
        /* new signal */
        drv_signal_create(&ctx->work_signal);
        
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
                const char *src = desc->thread_name ? desc->thread_name : "DRV";
                size_t b_count = strlen(ctx->thread_args[i].name);
                strncat(dst, src, b_count - 1);
        }


        /* thread id for main thread */
        thread_id_t th_id = drv_thread_id_self();
        
        ctx->main_thread = th_id;
        
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
        
        /* poke sleeping threads so we can shutdown */
        drv_signal_raise(&ctx->work_signal);

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
        drv_signal_destroy(&ctx->work_signal);
        
        /* destroy resources */
        if(desc->sched_free) {
                /* fiber data */
                for(i = 0; i < DRV_SCHED_MAX_FIBERS; ++i) {
                        desc->sched_free(ctx->fibers[i].stack);
                }
        
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
                thread_id_t th_id = drv_thread_id_self();
                int same_th = drv_thread_id_equal(th_id, ctx->main_thread);
                
                if(!same_th) {
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
        int i;

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

        if(DRV_SCHED_PCHECKS) {
                for(i = 0; i < desc->work_count; ++i) {
                        if(desc->work[i].func == 0) {
                                assert(!"DRV_SCHED_RESULT_INVALID_DESC");
                                return DRV_SCHED_RESULT_INVALID_DESC;
                        }
                }
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
        
        /* find thread */
        thread_id_t th_id = drv_thread_id_self();
        
        struct drv_thread_arg *tls = 0;
        
        for(i = 0; i < ctx->thread_count; ++i) {
                if(drv_thread_id_equal(ctx->thread_args[i].thread_id, th_id)) {
                        tls = &ctx->thread_args[i];
                        break;
                };
        };
        
        /* new marker */
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
        int count = ctx->work_count;
        for(i = 0; i < desc->work_count; ++i) {
                drv_task_fn fn  = desc->work[i].func;
                void *arg       = desc->work[i].arg;
                thread_id_t tid = desc->work[i].tid_lock ? th_id : 0;
                
                /* insert work */
                ctx->work[count + i].marker     = mk;
                ctx->work[count + i].arg        = arg;
                ctx->work[count + i].func       = fn;
                ctx->work[count + i].tid        = tid;
                ctx->work[count + i].blocked_mk = desc->blocked_mk;
        }
        
        ctx->work_count += desc->work_count;
        
        drv_signal_raise(&ctx->work_signal);
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
        
        /* we allow null markers because it cleans up calling code */
        if(wait_mk == 0) {
                return DRV_SCHED_RESULT_OK;
        }
        
        int i;

        /* find thread */
        thread_id_t th_id = drv_thread_id_self();
        
        struct drv_thread_arg *tls = 0;
        
        for(i = 0; i < ctx->thread_count; ++i) {
                if(drv_thread_id_equal(ctx->thread_args[i].thread_id, th_id)) {
                        tls = &ctx->thread_args[i];
                        break;
                };
        };
        
        /* block current task switch fiber back to thread_proc */
        if(tls) {
                /* switch back to thread proc */
                tls->work_fiber->blocked_marker = wait_mk;
                
                fcontext_t *fi_proc = &tls->work_fiber->context;
                fcontext_t th_proc = tls->home_context;

                if(DRV_PANIC_LOGGING) {
                        printf("DRV Wait %p Jump back to thread proc\n",
                                tls->work_fiber);
                }
                
                jump_fcontext(fi_proc, th_proc, NULL);
        }

        /* thread not in pool so spin until marker is done */
        else {
                for(;;) {
                        int found = 1;
        
                        drv_mutex_lock(&ctx->mut);

                        uint64_t instance = (wait_mk & 0xFFFFFFFF);
                        uint64_t idx = ((wait_mk >> 32) & 0xFFFFFFFF);

                        if(ctx->markers[idx].instance != instance) {
                                found = 0;
                        }

                        drv_mutex_unlock(&ctx->mut);
                
                        if(found) {
                                #ifdef _WIN32
                                /*Sleep(10);*/
                                #else
                                /*sleep(100);*/
                                #endif
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
