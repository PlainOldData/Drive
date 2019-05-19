

#include <drive/mem.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


/* ---------------------------------------------------------------- Config -- */


#ifndef DRV_MEM_PCHECKS
#define DRV_MEM_PCHECKS 1
#endif


#ifndef DRV_MEM_MAX_STACK_ALLOC
#define DRV_MEM_MAX_STACK_ALLOC 4
#endif


#ifndef DRV_MEM_MAX_STACK_VALLOC
#define DRV_MEM_MAX_STACK_VALLOC 4
#endif


#ifndef DRV_MEM_MAX_TAGGED_ALLOC
#define DRV_MEM_MAX_TAGGED_ALLOC 4
#endif


#ifndef DRV_MEM_MAX_TAGGED_VALLOC
#define DRV_MEM_MAX_TAGGED_VALLOC 4
#endif


/* ----------------------------------------------------- Drive mem Context -- */


struct drv_stack_alloc {
        int i;
};


struct drv_stack_valloc {
        int i;
};


struct drv_tag_alloc {
        int i;
};


struct drv_tag_valloc {
        int i;
};


struct drv_mem_ctx {
        struct drv_stack_alloc alloc_stack[DRV_MEM_MAX_STACK_ALLOC];
        struct drv_stack_valloc valloc_stack[DRV_MEM_MAX_STACK_VALLOC];
        struct drv_tag_alloc alloc_tagged[DRV_MEM_MAX_TAGGED_ALLOC];
        struct drv_tag_valloc valloc_tagged[DRV_MEM_MAX_TAGGED_VALLOC];

        drv_mem_alloc_fn alloc_fn;
        drv_mem_free_fn free_fn;
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

        if(DRV_MEM_PCHECKS && !desc->alloc_fn) {
                assert(!"DRV_MEM_RESULT_INVALID_DESC");
                return DRV_MEM_RESULT_INVALID_DESC;
        }

        struct drv_mem_ctx *ctx = desc->alloc_fn(sizeof(*ctx));

        if (!ctx) {
                return DRV_MEM_RESULT_FAIL;
        }

        ctx->alloc_fn = desc->alloc_fn;
        ctx->free_fn  = desc->free_fn;

        *out_ctx = ctx;

        return DRV_MEM_RESULT_OK;
}


drv_mem_result
drv_mem_ctx_destroy(
        struct drv_mem_ctx **destory_ctx)

{
        if(DRV_MEM_PCHECKS && !destory_ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !*destory_ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        /* get ctx */
        struct drv_mem_ctx *ctx = *destory_ctx;
        memset(ctx, 0, sizeof(*ctx));

        if(ctx->free_fn) {
                ctx->free_fn(ctx);
        }

        /* loop through allocators and clear memory */


        /* clear */
        *destory_ctx = 0;

        return DRV_MEM_RESULT_OK;
}


/* ------------------------------------------------------ General Allocate -- */


drv_mem_result
drv_mem_allocate(
        struct drv_mem_ctx *ctx,
        size_t bytes,
        void **out)
{
        if(DRV_MEM_PCHECKS && !ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !bytes) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        void *addr = ctx->alloc_fn(bytes);

        if (!addr) {
                return DRV_MEM_RESULT_FAIL;
        }

        *out = addr;

        return DRV_MEM_RESULT_OK;
}


drv_mem_result
drv_mem_free(
        struct drv_mem_ctx *ctx,
        void *addr)
{
        if(DRV_MEM_PCHECKS && !ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if (addr && ctx->free_fn) {
                ctx->free_fn(addr);
        }

        return DRV_MEM_RESULT_OK;
}


/* ------------------------------------------------------- Stack Allocator -- */


drv_mem_result
drv_mem_stack_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_stack_allocator_desc *desc,
        uint64_t *allocator_id)
{
        if(DRV_MEM_PCHECKS && !ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !desc) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !allocator_id) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        /* look for a free */
        if(desc->alloc_type == DRV_MEM_ALLOC_TYPE_PHYSICAL) {
                
        }
        else if(desc->alloc_type == DRV_MEM_ALLOC_TYPE_VIRTUAL) {

        }
        else {
                return DRV_MEM_RESULT_FAIL;
        }

        return DRV_MEM_RESULT_OK;
}


drv_mem_result
drv_mem_stack_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        unsigned bytes)
{
        (void)ctx;
        (void)alloc_id;
        (void)bytes;

        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_stack_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id)
{
        (void)ctx;
        (void)alloc_id;

        return DRV_MEM_RESULT_FAIL;
}


/* ------------------------------------------------------ Tagged Allocator -- */


drv_mem_result
drv_mem_tagged_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_tagged_allocator_desc *desc,
        uint64_t *allocator_id)
{
        (void)ctx;
        (void)desc;
        (void)allocator_id;

        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_tagged_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id,
        unsigned bytes)
{
        (void)ctx;
        (void)alloc_id;
        (void)tag_id;
        (void)bytes;

        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_tagged_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id)
{
        (void)ctx;
        (void)alloc_id;
        (void)tag_id;

        return DRV_MEM_RESULT_FAIL;
}


/* ---------------------------------------------------------------- Config -- */


#undef DRV_MEM_PCHECKS
#undef DRV_MEM_MAX_STACK_ALLOC
#undef DRV_MEM_MAX_STACK_VALLOC
#undef DRV_MEM_MAX_TAGGED_ALLOC
#undef DRV_MEM_MAX_TAGGED_VALLOC
