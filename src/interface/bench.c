#include <drive/bench.h>
#include <assert.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_BENCH_PCHECKS
#define DRV_BENCH_PCHECKS 1
#endif


/* --------------------------------------------------------- Platform Code -- */


#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
typedef pthread_mutex_t drv_mutex;
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
typedef CRITICAL_SECTION drv_mutex;
#endif


typedef union drv_atomic_int {
        void* align;
        int val;
} drv_atomic_int;


int
drv_atomic_int_load(
        drv_atomic_int *atomic)
{
        #if defined(__clang__) || defined(__GNUC__)
        return (int)__sync_fetch_and_add(&atomic->val, 0);
        #elif defined(_WIN32)
        return InterlockedCompareExchange((LONG*)&atomic->val, 0, 0);
        #endif
}


void
drv_atomic_int_store(
        drv_atomic_int *atomic,
        int val)
{
        #if defined(__clang__) || defined(__GNUC__)

        /*
        int ai_was = atomic->val;
        __sync_val_compare_and_swap(&atomic->val, ai_was, val);
        */

        /*
  __sync_lock_test_and_set(&atomic->val, val);
        __sync_lock_release(&atomic->val);
        */

        /* So aligned writes are atomic right? */
        atomic->val = val;

        #elif defined(_WIN32)
        InterlockedExchange((LONG*)&atomic->val, val);
        #endif
}


int
drv_atomic_int_inc(
        drv_atomic_int *atomic)
{
        #if defined(__clang__) || defined(__GNUC__)
        return (int)__sync_fetch_and_add(&atomic->val, 1);
        #elif defined(_WIN32)
        return InterlockedIncrement((LONG*)&atomic->val) - 1;
        #endif
}


/* -------------------------------------------------------------- Lifetime -- */


struct drv_bench_ctx {
        drv_atomic_int index;
        uint64_t size;
        struct drv_bench_info *info;
};


drv_bench_result
drv_bench_ctx_create(
        const struct drv_bench_create_ctx_desc *desc,
        struct drv_bench_ctx **out)
{
        /* param checks */
        if(DRV_BENCH_PCHECKS && !desc) {
                assert(!"DRV_BENCH_RESULT_BAD_PARAM");
                return DRV_BENCH_RESULT_BAD_PARAM;
        }

        if(DRV_BENCH_PCHECKS && !desc->ring_buf) {
                assert(!"DRV_BENCH_RESULT_INVALID_DESC");
                return DRV_BENCH_RESULT_INVALID_DESC;
        }

        if(DRV_BENCH_PCHECKS && !desc->ring_buf_size) {
                assert(!"DRV_BENCH_RESULT_INVALID_DESC");
                return DRV_BENCH_RESULT_INVALID_DESC;
        }

        if(DRV_BENCH_PCHECKS && !desc->alloc) {
                assert(!"DRV_BENCH_RESULT_INVALID_DESC");
                return DRV_BENCH_RESULT_INVALID_DESC;
        }

        struct drv_bench_ctx *ctx = desc->alloc(sizeof(*ctx));

        if(!ctx) {
                assert(!"DRV_BENCH_RESULT_FAIL");
                return DRV_BENCH_RESULT_FAIL;
        }

        drv_atomic_int_store(&ctx->index, 0);
        ctx->size = desc->ring_buf_size / sizeof(*ctx);
        ctx->info = desc->ring_buf;
        
        *out = ctx;

        return DRV_BENCH_RESULT_OK;
}


drv_bench_result
drv_bench_ctx_destroy(
        const struct drv_bench_destroy_ctx_desc *desc,
        struct drv_bench_ctx **in_out)
{
        if(DRV_BENCH_PCHECKS && !in_out) {
                assert(!"DRV_BENCH_RESULT_BAD_PARAM");
                return DRV_BENCH_RESULT_BAD_PARAM;
        }

        if(DRV_BENCH_PCHECKS && !*in_out) {
                assert(!"DRV_BENCH_RESULT_BAD_PARAM");
                return DRV_BENCH_RESULT_BAD_PARAM;
        }

        struct drv_bench_ctx *ctx = *in_out;

        if(desc->free_fn) {
                desc->free_fn(ctx);
        }

        *in_out = 0;

        return DRV_BENCH_RESULT_OK;
}


/* --------------------------------------------------------- Add new bench -- */


struct drv_bench_info*
drv_bench_event_new(
        struct drv_bench_ctx *ctx)
{
        int idx = drv_atomic_int_inc(&ctx->index);
        idx = idx % ctx->size;

        return &ctx->info[idx];
}


drv_bench_result
drv_bench_event_fetch(
        struct drv_bench_ctx *ctx,
        struct drv_bench_info *out_data,
        size_t *out_size)
{
        if(out_data) {
                
        }
        
        if(out_size) {
                *out_size = (size_t)ctx->size;
        }
        
        return DRV_BENCH_RESULT_FAIL;
}


/* --------------------------------------------------------------- Convert -- */


drv_bench_result
drv_bench_convert_to_trace(
        struct drv_bench_ctx *ctx,
        const struct drv_bench_to_trace *desc)
{
        (void)ctx;
        (void)desc;
        
        return DRV_BENCH_RESULT_FAIL;
}
