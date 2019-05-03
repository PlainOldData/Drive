#ifndef DRV_ALLOC_INCLUED_BAA7D4EC_7559_4079_A6BA_86633231A05F
#define DRV_ALLOC_INCLUED_BAA7D4EC_7559_4079_A6BA_86633231A05F


#include <stdint.h>


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


/* -------------------------------------------------------------- Lifetime -- */


typedef void*(*drv_mem_alloc_fn)(unsigned long);
typedef void(*drv_mem_free_fn)(void *);


struct drv_mem_ctx_create_desc {
        unsigned max_bytes;  /* must be greater than zero */
        drv_mem_alloc_fn alloc_fn; /* default malloc */
        drv_mem_free_fn free_fn; /* default free */
};


drv_mem_result
drv_mem_ctx_create(
        const struct drv_mem_ctx_create_desc *desc,
        struct drv_mem_ctx **out_ctx);


drv_mem_result
drv_mem_ctx_destroy(
        struct drv_mem_ctx **destory_ctx);


/* ------------------------------------------------------ General Allocate -- */


drv_mem_result
drv_mem_allocate(
        struct drv_mem_ctx *ctx,
        unsigned bytes,
        void **out);


drv_mem_result
drv_mem_free(
        struct drv_mem_ctx *ctx,
        void *addr);


/* ------------------------------------------------------- Stack Allocator -- */


struct drv_mem_stack_allocator_desc {
        unsigned size_of_stack;
        int ring_buffer;
};


drv_mem_result
drv_mem_stack_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_stack_allocator_desc *desc,
        uint64_t *allocator_id);


drv_mem_result
drv_mem_stack_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        unsigned bytes);


drv_mem_result
drv_mem_stack_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id);


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
        unsigned bytes);


drv_mem_result
drv_mem_tagged_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id);


#ifdef __cplusplus
} /* extern */
#endif


#endif
