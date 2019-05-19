#include <drive/bench.h>
#include <stdio.h>
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
#include <sys/timeb.h>
#include <DbgHelp.h>
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

        uint64_t start_ts;
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
        ctx->size = desc->ring_buf_size;
        ctx->info = desc->ring_buf;

        #ifdef _WIN32
        struct _timeb tb;
        _ftime64_s(&tb);
        uint64_t ts = (uint64_t)((tb.time * 1000L) + tb.millitm);
        #endif
        
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

        #ifdef _WIN32
        struct _timeb tb;
        _ftime64_s(&tb);
        uint64_t ts = (uint64_t)((tb.time * 1000L) + tb.millitm);
        
        ctx->info[idx].ts = ts - ctx->start_ts;
        #endif

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
        FILE *f = fopen(desc->file, "w");
        if(f == NULL) {
                assert(!"DRV_BENCH_RESULT_FAIL");
                return DRV_BENCH_RESULT_FAIL;
        }

        #ifdef _WIN32
        SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME);
        BOOL initd = SymInitialize(GetCurrentProcess(),NULL,TRUE);
        int pid = (int)GetCurrentProcessId();
        #endif
        
        int pid = 0;

        fprintf(f, "{\n\t\"traceEvents\": [\n");

        uint64_t idx = drv_atomic_int_load(&ctx->index);
        uint64_t start = idx > ctx->size ? (idx + 1) % ctx->size : 0;
        uint64_t count = idx > ctx->size ? ctx->size : idx;
        uint64_t i;

        for(i = 0; i < count; ++i) {
                int j = (start + i) % count;

                struct drv_bench_info *st_evt = &ctx->info[j];
                struct drv_bench_info *ed_evt = 0;

                if(st_evt->id == DRV_BENCH_EVENT_ID_START) {
                        /* look for end */
                        uint64_t k;
                        for(k = i + 1; k < count; ++k) {
                                uint64_t l = (start + k) % count;

                                struct drv_bench_info *evt = &ctx->info[l];

                                if(evt->pair_ident == st_evt->pair_ident) {
                                        if(evt->id == DRV_BENCH_EVENT_ID_END) {
                                                ed_evt = evt;
                                                break;
                                        }
                                }
                        }
                }

                /* no end event, move on to the next one */
                if(!ed_evt) {
                        continue;
                }

                const char *name = st_evt->name ? st_evt->name : "Unknown func";

                #ifdef _WIN32
                if(ed_evt->flgs & DRV_BENCH_EVENT_EXTRA_TO_SYMBOL_NAME) {
                        DWORD64  dwDisplacement = 0;
                        char buffer[sizeof(SYMBOL_INFO) + 10000 * sizeof(TCHAR)] = {0};
                        PSYMBOL_INFO sym = (PSYMBOL_INFO)buffer;

                        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
                        sym->MaxNameLen = 256;

                        SymFromAddr(GetCurrentProcess(), (DWORD64)st_evt->extra, &dwDisplacement, sym);
                        name = sym->Name;
                }
                #endif

                fprintf(f, "\t\t{\n");
                fprintf(f, "\t\t\t\"name\": \"%s\",\n", name);
                fprintf(f, "\t\t\t\"cat\": \"%s\",\n", st_evt->category);
                fprintf(f, "\t\t\t\"ph\": \"%s\",\n", "X");
                fprintf(f, "\t\t\t\"ts\": %f,\n", (float)(st_evt->ts) / 1000.f);
                fprintf(f, "\t\t\t\"dur\": %f,\n", (float)(ed_evt->ts - st_evt->ts) / 1000.f);
                fprintf(f, "\t\t\t\"pid\": %d,\n", pid);
                fprintf(f, "\t\t\t\"tid\": %lu\n", st_evt->tid);
                fprintf(f, "\t\t},\n");
        }

        /* erase last comma */
        fseek(f, -(strlen(",\n") + 1), SEEK_CUR);

        fprintf(f, "\n\t],\n");
        fprintf(f, "\t\"displayTimeUnit\": \"ns\"\n}\n");

        if (fclose(f) != 0) {
                assert(!"DRV_BENCH_RESULT_FAIL");
                return DRV_BENCH_RESULT_FAIL;
        }
        
        return DRV_BENCH_RESULT_OK;
}
