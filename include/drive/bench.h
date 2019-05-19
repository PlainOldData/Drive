#ifndef DRV_BENCH_INCLUDED_0D71CD17_BF04_4499_A8F3_FF99BF7266BD
#define DRV_BENCH_INCLUDED_0D71CD17_BF04_4499_A8F3_FF99BF7266BD


#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


struct drv_bench_ctx; /* opaque type */


typedef enum _drv_bench_result {
        DRV_BENCH_RESULT_OK,
        DRV_BENCH_RESULT_BAD_PARAM,
        DRV_BENCH_RESULT_INVALID_DESC,
        DRV_BENCH_RESULT_FAIL,
} drv_bench_result;


typedef enum _drv_bench_event_id {
        DRV_BENCH_EVENT_ID_START,
        DRV_BENCH_EVENT_ID_END,
} drv_bench_event_id;


typedef enum _drv_bench_event_flags {
        DRV_BENCH_EVENT_EXTRA_TO_SYMBOL_NAME = 1 << 0,
} drv_bench_event_flags;


struct drv_bench_info {
        drv_bench_event_id id;
        drv_bench_event_flags flgs;
        uint64_t ts;
        uint32_t tid;
        const char *name;
        const char *category;
        void *extra;
        uint64_t pair_ident;
};


/* -------------------------------------------------------------- Lifetime -- */


typedef void*(*drv_bench_alloc_fn)(size_t);
typedef void(*drv_bench_free_fn)(void *);


struct drv_bench_create_ctx_desc {
        struct drv_bench_info *ring_buf; /* required to not be null */
        size_t ring_buf_size;            /* required to be > 0 */
        drv_bench_alloc_fn alloc;        /* required */
};


/*
 * returns DRV_BENCH_RESULT_BAD_PARAM if desc is null
 * returns DRV_BENCH_RESULT_BAD_PARAM if out is null
 * returns DRV_BENCH_RESULT_INVALID_DESC if desc.ring_buffer is null
 * returns DRV_BENCH_RESULT_INVALID_DESC if desc.ring_buffer_size is 0
 * returns DRV_BENCH_RESULT_FAIL if desc.alloc returns null
 */
drv_bench_result
drv_bench_ctx_create(
        const struct drv_bench_create_ctx_desc *desc,
        struct drv_bench_ctx **out);


struct drv_bench_destroy_ctx_desc {
        drv_bench_free_fn free_fn;      /* optional, will not free if null */
};


/*
 * returns DRV_BENCH_RESULT_BAD_PARAM if desc is null
 * returns DRV_BENCH_RESULT_BAD_PARAM if in_out is null
 */
drv_bench_result
drv_bench_ctx_destroy(
        const struct drv_bench_destroy_ctx_desc *desc,
        struct drv_bench_ctx **in_out);


/* --------------------------------------------------------- Add new bench -- */


/*
 * No error checks, returns the next item in the ring buffer.
 */
struct drv_bench_info*
drv_bench_event_new(
        struct drv_bench_ctx *ctx);


/* ------------------------------------------------------------------ View -- */


drv_bench_result
drv_bench_event_view(
        struct drv_bench_ctx *ctx,
        struct drv_bench_info *out_buffer,
        unsigned *out_start,
        unsigned *out_count);


/* --------------------------------------------------------------- Convert -- */
/*
 * Helper functions to convert data.
 */


typedef enum _drv_bench_time {
        DRV_BENCH_TIME_NS,
        DRV_BENCH_TIME_MS,
} drv_bench_time;


struct drv_bench_to_trace {
        const char *file;
        drv_bench_time time_unit;
};


drv_bench_result
drv_bench_convert_to_trace(
        struct drv_bench_ctx *ctx,
        const struct drv_bench_to_trace *desc);


#ifdef __cplusplus
} /* extern */
#endif


#endif
