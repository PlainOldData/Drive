

#include <drive/mem.h>
#include <assert.h>
#include <stdlib.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_MEM_PCHECKS
#define DRV_MEM_PCHECKS 1
#endif


/* ----------------------------------------------------- Drive mem Context -- */


struct drv_mem_ctx {
        int i;
};


/* -------------------------------------------------------------- Lifetime -- */


drv_mem_result
drv_mem_ctx_create(
        const struct drv_mem_ctx_create_desc *desc,
        struct drv_mem_ctx **out_ctx)
{
        if(DRV_MEM_PCHECKS && !desc) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !out_ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !desc->max_bytes) {
                assert(!"DRV_MEM_RESULT_INVALID_DESC");
                return DRV_MEM_RESULT_INVALID_DESC;
        }

        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_ctx_destroy(
        struct drv_mem_ctx **destory_ctx)

{
        if(DRV_MEM_PCHECKS && !destory_ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        return DRV_MEM_RESULT_FAIL;
}


/* ------------------------------------------------------ General Allocate -- */


drv_mem_result
drv_mem_allocate(
        struct drv_mem_ctx *ctx,
        unsigned bytes,
        void **out)
{
        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_free(
        struct drv_mem_ctx *ctx,
        void *addr)
{
        return DRV_MEM_RESULT_FAIL;
}


/* ------------------------------------------------------- Stack Allocator -- */


drv_mem_result
drv_mem_stack_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_stack_allocator_desc *desc,
        uint64_t *allocator_id)
{
        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_stack_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        unsigned bytes)
{
        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_stack_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id)
{
        return DRV_MEM_RESULT_FAIL;
}


/* ------------------------------------------------------ Tagged Allocator -- */


drv_mem_result
drv_mem_tagged_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_tagged_allocator_desc *desc,
        uint64_t *allocator_id)
{
        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_tagged_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id,
        unsigned bytes)
{
        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_tagged_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id)
{
        return DRV_MEM_RESULT_FAIL;
}


/* ---------------------------------------------------------------- Config -- */


#undef DRV_MEM_PCHECKS
