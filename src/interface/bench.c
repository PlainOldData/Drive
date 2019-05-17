#include <drive/bench.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_BENCH_PCHECKS
#define DRV_BENCH_PCHECKS 1
#endif


/* -------------------------------------------------------------- Lifetime -- */


struct drv_bench_ctx {
        size_t index;
        size_t size;
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

        if(DRV_BENCH_PCHECKS && !desc->bring_buf) {
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

        ctx->index = 0;
        ctx->size = desc->ring_buf_size / sizeof(*ctx);
        ctx->info = desc->ring_buf;

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
        /* increment index */
        int idx = ctx->index % ctx->size;
        ctx->index++;

        return &ctx->info[idx];
}


/* --------------------------------------------------------------- Convert -- */


drv_bench_result
drv_bench_convert_to_trace(
        struct drv_bench_ctx *ctx,
        const struct drv_bench_to_trace *desc)
{
        return DRV_BENCH_RESULT_FAIL;
}