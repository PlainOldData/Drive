#ifndef DRV_ALLOC_INCLUED_BAA7D4EC_7559_4079_A6BA_86633231A05F
#define DRV_ALLOC_INCLUED_BAA7D4EC_7559_4079_A6BA_86633231A05F


#ifndef __cplusplus
#include <stdint.h>
#include <stddef.h>
#else
#include <cstdint>
#include <cstddef>
#endif


#ifdef __cplusplus
extern "C" {
#endif


struct drv_mem_ctx; /* opaque */


typedef enum _drv_mem_result {
        DRV_MEM_RESULT_OK,
        DRV_MEM_RESULT_BAD_PARAM,
        DRV_MEM_RESULT_INVALID_DESC,
        DRV_MEM_RESULT_OUT_OF_RESOURCES,
        DRV_MEM_RESULT_FAIL,
} drv_mem_result;


typedef enum _drv_mem_alloc_type {
        DRV_MEM_ALLOC_TYPE_PHYSICAL,
        DRV_MEM_ALLOC_TYPE_VIRTUAL,
} drv_mem_alloc_type;


/* -------------------------------------------------------------- Lifetime -- */


typedef void*(*drv_mem_alloc_fn)(size_t);
typedef void(*drv_mem_free_fn)(void *);


struct drv_mem_ctx_create_desc {
        drv_mem_alloc_fn alloc_fn;
        drv_mem_free_fn free_fn;
};


/*
 * returns DRV_MEM_RESULT_BAD_PARAM if desc is null.
 * returns DRV_MEM_RESULT_BAD_PARAM if out is null.
 * returns DRV_MEM_RESULT_INVALID_DESC if desc.alloc_fn is null.
 * returns DRV_MEM_RESULT_INVALID_DESC if desc.free_fn is null.
 * returns DRV_MEM_RESULT_FAIL on internal failure.
 * returns DRV_MEM_RESULT_OK on success.
 */
drv_mem_result
drv_mem_ctx_create(
        const struct drv_mem_ctx_create_desc *desc,
        struct drv_mem_ctx **out);


/*
 * returns DRV_MEM_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_MEM_RESULT_FAIL on internal failure.
 * returns DRV_MEM_RESULT_OK on success.
 */
drv_mem_result
drv_mem_ctx_destroy(
        struct drv_mem_ctx **ctx);


/* ------------------------------------------------------ General Allocate -- */


/*
 * returns DRV_MEM_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_MEM_RESULT_FAIL on internal failure.
 * returns DRV_MEM_RESULT_OK on success.
 */
drv_mem_result
drv_mem_allocate(
        struct drv_mem_ctx *ctx,
        size_t bytes,
        void **out);


/*
 * returns DRV_MEM_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_MEM_RESULT_FAIL on internal failure.
 * returns DRV_MEM_RESULT_OK on success.
 */
drv_mem_result
drv_mem_free(
        struct drv_mem_ctx *ctx,
        void *addr);


/* ------------------------------------------------------- Stack Allocator -- */


struct drv_mem_stack_allocator_desc {
        size_t size_of_stack;
        drv_mem_alloc_type alloc_type;
};


/*
 * returns DRV_MEM_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_MEM_RESULT_BAD_PARAM if desc is null.
 * returns DRV_MEM_RESULT_BAD_PARAM if allocator_id is null.
 * returns DRV_MEM_RESULT_INVALID_DESC if desc.size_of_stack is 0.
 * returns DRV_MEM_RESULT_FAIL on internal failure.
 * returns DRV_MEM_RESULT_OK on success.
 */
drv_mem_result
drv_mem_stack_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_stack_allocator_desc *desc,
        uint64_t *allocator_id);


/*
 * returns DRV_MEM_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_MEM_RESULT_BAD_PARAM if allocator_id is 0.
 * returns DRV_MEM_RESULT_BAD_PARAM if out_mem is null.
 * returns DRV_MEM_RESULT_FAIL if allocator_id is invalid.
 * returns DRV_MEM_RESULT_FAIL on internal failure.
 * returns DRV_MEM_RESULT_OK on success.
 */
drv_mem_result
drv_mem_stack_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t allocator_id,
        size_t bytes,
        void **out_mem);


/*
 * returns DRV_MEM_RESULT_BAD_PARAM if ctx is null.
 * returns DRV_MEM_RESULT_BAD_PARAM if allocator_id is 0.
 * returns DRV_MEM_RESULT_FAIL if allocator_id is invalid.
 * returns DRV_MEM_RESULT_FAIL on internal failure.
 * returns DRV_MEM_RESULT_OK on success.
 */
drv_mem_result
drv_mem_stack_clear(
        struct drv_mem_ctx *ctx,
        uint64_t allocator_id);


/* ------------------------------------------------------ Tagged Allocator -- */


struct drv_mem_tagged_allocator_desc {
        unsigned chunk_count;
        unsigned chunk_size;
};


drv_mem_result
drv_mem_tagged_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_tagged_allocator_desc *desc,
        uint64_t *allocator_id);


drv_mem_result
drv_mem_tagged_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id,
        size_t bytes);


drv_mem_result
drv_mem_tagged_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id);


#ifdef __cplusplus
} /* extern */
#endif


#endif
