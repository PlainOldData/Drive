

#include <drive/mem.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(__APPLE__) || (__linux__)
#include <unistd.h>
#include <sys/mman.h>
#endif


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
        void *start;
        size_t curr;
        size_t bytes;
};


struct drv_stack_valloc {
        #ifdef _WIN32
        LPVOID start;
        DWORD page_size;
        SIZE_T pages_reserved;
        SIZE_T bytes;
        SIZE_T curr;
        #elif defined(__APPLE__) || (__linux__)
        void *start;
        long page_size;
        size_t pages_reserved;
        size_t bytes;
        size_t curr;
        #else
        #error "No Impl"
        #endif
};


struct drv_tag_alloc {
        char *start;
        uint64_t *alloc_ids;

        size_t chunk_count;
        size_t chunk_bytes;
};


struct drv_mem_ctx {
        struct drv_stack_alloc alloc_stack[DRV_MEM_MAX_STACK_ALLOC];
        struct drv_stack_valloc valloc_stack[DRV_MEM_MAX_STACK_VALLOC];
        struct drv_tag_alloc alloc_tagged[DRV_MEM_MAX_TAGGED_ALLOC];

        drv_mem_alloc_fn alloc_fn;
        drv_mem_free_fn free_fn;
};


/* allocator ids */
#define DRV_MEM_ALLOC_STACK 1
#define DRV_MEM_ALLOC_VSTACK 2
#define DRV_MEM_ALLOC_TAGGED 3
#define DRV_MEM_ALLOC_VTAGGED 4


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

        if(DRV_MEM_PCHECKS && !desc->free_fn) {
                assert(!"DRV_MEM_RESULT_INVALID_DESC");
                return DRV_MEM_RESULT_INVALID_DESC;
        }

        struct drv_mem_ctx *ctx = desc->alloc_fn(sizeof(*ctx));
        memset(ctx, 0, sizeof(*ctx));

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

        if(ctx->free_fn) {
                int i;

                for(i = 0; i < DRV_MEM_MAX_STACK_ALLOC; ++i) {
                        if(ctx->alloc_stack[i].start) {
                                ctx->free_fn(ctx->alloc_stack[i].start);
                        }
                }

                for(i = 0; i < DRV_MEM_MAX_STACK_VALLOC; ++i) {
                        #ifdef _WIN32
                        if(ctx->valloc_stack[i].start) {
                                BOOL ok = VirtualFree(
                                        ctx->valloc_stack[i].start,
                                        0,
                                        MEM_RELEASE);
                                
                                if (!ok) {
                                        DWORD err = GetLastError();
                                        assert(!"Failed in release" && err);
                                }
                        }
                        #elif defined(__APPLE__) || (__linux__)
                        
                        #endif
                }

                for(i = 0; i < DRV_MEM_MAX_TAGGED_ALLOC; ++i) {
                        if(ctx->alloc_tagged[i].start) {
                                ctx->free_fn(ctx->alloc_tagged[i].start);
                                ctx->free_fn(ctx->alloc_tagged[i].alloc_ids);
                        }
                }

                ctx->free_fn(ctx);
        }

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

        if(!bytes) {
                *out = 0;
                return DRV_MEM_RESULT_OK;
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

        if (!addr) {
                return DRV_MEM_RESULT_OK;
        }

        ctx->free_fn(addr);

        return DRV_MEM_RESULT_OK;
}


/* ------------------------------------------------------- Stack Allocator -- */


drv_mem_result
drv_mem_stack_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_stack_allocator_desc *desc,
        uint64_t *allocator_id)
{
        /* param check */
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
        
        if(DRV_MEM_PCHECKS && !desc->size_of_stack) {
                assert(!"DRV_MEM_RESULT_INVALID_DESC");
                return DRV_MEM_RESULT_INVALID_DESC;
        }

        /* physical stack allocator */
        if(desc->alloc_type == DRV_MEM_ALLOC_TYPE_PHYSICAL) {

                /* find a free allocator */
                int i;
                struct drv_stack_alloc *alloc = 0;
                
                for(i = 0; i < DRV_MEM_MAX_STACK_ALLOC; ++i) {
                        if(!ctx->alloc_stack[i].start) {
                                alloc = &ctx->alloc_stack[i];
                                break;
                        }
                }
                
                if(!alloc) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                void *start = ctx->alloc_fn(desc->size_of_stack);
                
                if(!start) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                /* setup */
                alloc->bytes = desc->size_of_stack;
                alloc->start = start;
                
                /* allocator ID */
                uint64_t alloc_type = DRV_MEM_ALLOC_STACK;
                uint32_t alloc_idx  = i;
                uint64_t alloc_id   = (alloc_type << 32) | alloc_idx;
                
                *allocator_id = alloc_id;
                
                return DRV_MEM_RESULT_OK;
        }
        /* virtual memory allocator */
        else if(desc->alloc_type == DRV_MEM_ALLOC_TYPE_VIRTUAL) {

                /* find a free allocator */
                int i;
                struct drv_stack_valloc *alloc = 0;
                
                for(i = 0; i < DRV_MEM_MAX_STACK_ALLOC; ++i) {
                        if(!ctx->alloc_stack[i].start) {
                                alloc = &ctx->valloc_stack[i];
                                break;
                        }
                }
                
                if(!alloc) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                #ifdef _WIN32
                SYSTEM_INFO sys_info;
                GetSystemInfo(&sys_info);

                LPVOID addr = VirtualAlloc(
                        NULL,
                        desc->size_of_stack,
                        MEM_RESERVE,
                        PAGE_NOACCESS);

                if(!addr) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                /* setup */
                alloc->page_size      = sys_info.dwPageSize;
                alloc->bytes          = desc->size_of_stack;
                alloc->curr           = 0;
                alloc->pages_reserved = 0;
                alloc->start          = addr;
                #elif defined(__APPLE__) || defined(__linux__)
                long page_size = sysconf(_SC_PAGE_SIZE);
                
                int prot = PROT_READ | PROT_WRITE;
                int flags = MAP_SHARED | MAP_ANONYMOUS;
                
                void *addr = mmap(
                        NULL,
                        desc->size_of_stack,
                        prot,
                        flags,
                        0,
                        0);
                
                assert(addr != (void*)-1);
                
                alloc->start          = addr;
                alloc->curr           = 0;
                alloc->page_size      = page_size;
                alloc->pages_reserved = 0;
                alloc->bytes          = desc->size_of_stack;
                #endif

                /* allocator ID */
                uint64_t alloc_type = DRV_MEM_ALLOC_VSTACK;
                uint32_t alloc_idx  = i;
                uint64_t alloc_id   = (alloc_type << 32) | alloc_idx;
                
                *allocator_id = alloc_id;

                return DRV_MEM_RESULT_OK;
        }

        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_stack_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        size_t bytes,
        void **out_mem)
{
        /* param check */
        if(DRV_MEM_PCHECKS && !ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !alloc_id) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !out_mem) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(bytes == 0) {
                *out_mem = 0;
                return DRV_MEM_RESULT_OK;
        }
        
        uint64_t idx = (alloc_id & 0xFFFFFFFF);
        uint64_t type = ((alloc_id >> 32) & 0xFFFFFFFF);
        
        if(type != DRV_MEM_ALLOC_STACK && type != DRV_MEM_ALLOC_VSTACK) {
                assert(!"DRV_MEM_RESULT_FAIL");
                return DRV_MEM_RESULT_FAIL;
        }

        if(type == DRV_MEM_ALLOC_STACK) {
                if(DRV_MEM_PCHECKS && idx >= DRV_MEM_MAX_STACK_ALLOC) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                struct drv_stack_alloc *alloc = &ctx->alloc_stack[idx];
                
                if(bytes + alloc->curr > alloc->bytes) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                size_t old   = alloc->curr;
                alloc->curr += bytes;
                char *store  = alloc->start;
                *out_mem     = (void*)(store + old);
                
                return DRV_MEM_RESULT_OK;
        }
        else if(type == DRV_MEM_ALLOC_VSTACK) {
                if(DRV_MEM_PCHECKS && idx >= DRV_MEM_MAX_STACK_VALLOC) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                struct drv_stack_valloc *alloc = &ctx->valloc_stack[idx];
                
                #ifdef _WIN32
                if((bytes + alloc->curr) > alloc->bytes) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                /* allocate more pages */
                SIZE_T bytes_need = bytes + alloc->curr;
                SIZE_T page_si = alloc->page_size;
                SIZE_T page_re = alloc->pages_reserved;
                SIZE_T bytes_alloced = page_re * page_si;

                if(bytes_need > bytes_alloced) {
                        SIZE_T pages = 1;

                        while(bytes_need > (page_re + pages) * page_si) {
                                pages += 1;
                        }

                        char *next = (char*)alloc->start;
                        next += (alloc->pages_reserved * alloc->page_size);

                        alloc->pages_reserved += pages;

                        LPVOID addr = VirtualAlloc(
                                (LPVOID)next,
                                alloc->page_size * pages,
                                MEM_COMMIT,
                                PAGE_READWRITE);

                        /* we only need to hold onto start */
                        (void)addr;
                }
                
                SIZE_T old = alloc->curr;
                alloc->curr += bytes;
                char *store = alloc->start;
                *out_mem = (void*)(store + old);
                #elif defined(__APPLE__) || (__linux__)
                if((bytes + alloc->curr) > alloc->bytes) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                /* allocate more pages */
                size_t bytes_need = bytes + alloc->curr;
                size_t page_si = alloc->page_size;
                size_t page_re = alloc->pages_reserved;
                size_t bytes_alloced = page_re * page_si;

                if(bytes_need > bytes_alloced) {
                        size_t pages = 1;

                        while(bytes_need > (page_re + pages) * page_si) {
                                pages += 1;
                        }

                        char *next = (char*)alloc->start;
                        next += (alloc->pages_reserved * alloc->page_size);

                        alloc->pages_reserved += pages;
                        
                        int prot = PROT_READ | PROT_WRITE;
                        int flags = MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED;

                        void *addr = mmap(
                                next,
                                pages * alloc->page_size,
                                prot,
                                flags,
                                0,
                                0);

                        /* we only need to hold onto start */
                        assert(addr != (void*)-1);
                }
                
                size_t old = alloc->curr;
                alloc->curr += bytes;
                char *store = alloc->start;
                *out_mem = (void*)(store + old);
                #endif

                return DRV_MEM_RESULT_OK;
        }
        
        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_stack_clear(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id)
{
        /* param check */
        if(DRV_MEM_PCHECKS && !ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !alloc_id) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }
        
        uint64_t idx = (alloc_id & 0xFFFFFFFF);
        uint64_t type = ((alloc_id >> 32) & 0xFFFFFFFF);
        
        if(type != DRV_MEM_ALLOC_STACK && type != DRV_MEM_ALLOC_VSTACK) {
                assert(!"DRV_MEM_RESULT_FAIL");
                return DRV_MEM_RESULT_FAIL;
        }

        if(type == DRV_MEM_ALLOC_STACK) {
                if(DRV_MEM_PCHECKS && idx >= DRV_MEM_MAX_STACK_ALLOC) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                struct drv_stack_alloc *alloc = &ctx->alloc_stack[idx];
                
                alloc->curr = 0;
                
                return DRV_MEM_RESULT_OK;
        }
        else if(type == DRV_MEM_ALLOC_VSTACK) {
                if(DRV_MEM_PCHECKS && idx >= DRV_MEM_MAX_STACK_VALLOC) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                struct drv_stack_valloc *alloc = &ctx->valloc_stack[idx];
                
                #ifdef _WIN32
                BOOL ok = VirtualFree(alloc->start, 0, MEM_DECOMMIT);
                alloc->curr = 0;
                alloc->pages_reserved = 0;

                if(!ok) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                #elif defined(__APPLE__) || (__linux__)
                if(alloc->pages_reserved) {
//                        int ok = munmap(alloc->start, alloc->bytes);
                        alloc->curr = 0;
                        alloc->pages_reserved = 0;
                        
//                        if(ok != 0) {
//                                assert(!"DRV_MEM_RESULT_FAIL");
//                                return DRV_MEM_RESULT_FAIL;
//                        }
                }
                #endif
                
                return DRV_MEM_RESULT_OK;
        }
        
        return DRV_MEM_RESULT_FAIL;
}


/* ------------------------------------------------------ Tagged Allocator -- */


drv_mem_result
drv_mem_tagged_allocator_create(
        struct drv_mem_ctx *ctx,
        struct drv_mem_tagged_allocator_desc *desc,
        uint64_t *allocator_id)
{
        /* param check */
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
        
        if(DRV_MEM_PCHECKS && !desc->chunk_size) {
                assert(!"DRV_MEM_RESULT_INVALID_DESC");
                return DRV_MEM_RESULT_INVALID_DESC;
        }

        if(DRV_MEM_PCHECKS && !desc->chunk_count) {
                assert(!"DRV_MEM_RESULT_INVALID_DESC");
                return DRV_MEM_RESULT_INVALID_DESC;
        }

        if(DRV_MEM_PCHECKS && desc->alloc_type != DRV_MEM_ALLOC_TYPE_PHYSICAL) {
                assert(!"DRV_MEM_RESULT_INVALID_DESC");
                return DRV_MEM_RESULT_INVALID_DESC;
        }

        /* physical stack allocator */
        if(desc->alloc_type == DRV_MEM_ALLOC_TYPE_PHYSICAL) {

                /* find a free allocator */
                unsigned i;
                uint32_t idx = 0;

                struct drv_tag_alloc *alloc = 0;
                
                for(i = 0; i < DRV_MEM_MAX_TAGGED_ALLOC; ++i) {
                        if(!ctx->alloc_tagged[i].start) {
                                alloc = &ctx->alloc_tagged[i];
                                idx = i;

                                break;
                        }
                }
                
                if(!alloc) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                unsigned count = desc->chunk_count;

                unsigned chunk_bytes = count * desc->chunk_size;
                void *start          = ctx->alloc_fn(chunk_bytes);

                if(!start) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                unsigned id_bytes = count * sizeof(alloc->alloc_ids[0]);
                uint64_t *ids     = ctx->alloc_fn(id_bytes);

                if(!ids) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }

                for(i = 0; i < count; ++i) {
                        ids[i] = 0;
                }

                /* setup */
                alloc->start       = start;
                alloc->alloc_ids   = ids;
                alloc->chunk_bytes = desc->chunk_size;
                alloc->chunk_count = count;

                /* allocator ID */
                uint64_t alloc_type = DRV_MEM_ALLOC_TAGGED;
                uint32_t alloc_idx  = idx;
                uint64_t alloc_id   = (alloc_type << 32) | alloc_idx;
                
                *allocator_id = alloc_id;
                
                return DRV_MEM_RESULT_OK;
        }

        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_tagged_alloc(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id,
        void **out_mem,
        size_t *out_bytes)
{
        /* param check */
        if(DRV_MEM_PCHECKS && !ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !alloc_id) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !out_mem) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }
        
        uint64_t idx = (alloc_id & 0xFFFFFFFF);
        uint64_t type = ((alloc_id >> 32) & 0xFFFFFFFF);
        
        if(type != DRV_MEM_ALLOC_TAGGED && type != DRV_MEM_ALLOC_VTAGGED) {
                assert(!"DRV_MEM_RESULT_FAIL");
                return DRV_MEM_RESULT_FAIL;
        }

        if(type == DRV_MEM_ALLOC_TAGGED) {
                if(DRV_MEM_PCHECKS && idx >= DRV_MEM_MAX_TAGGED_ALLOC) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                /* search for a free space */
                struct drv_tag_alloc *alloc = &ctx->alloc_tagged[idx];
                
                size_t i;
                void *addr = 0;

                for(i = 0; i < alloc->chunk_count; ++i) {
                        uint64_t id = alloc->alloc_ids[i];
                        if(id == 0) {
                                size_t chunk_start = alloc->chunk_bytes * i;
                                addr = &alloc->start[chunk_start];

                                alloc->alloc_ids[i] = tag_id;

                                break;
                        }
                }

                *out_mem = addr;
                
                if (out_bytes) {
                        *out_bytes = addr ? alloc->chunk_bytes : 0;
                }

                return DRV_MEM_RESULT_OK;
        }

        return DRV_MEM_RESULT_FAIL;
}


drv_mem_result
drv_mem_tagged_free(
        struct drv_mem_ctx *ctx,
        uint64_t alloc_id,
        uint64_t tag_id)
{
       /* param check */
        if(DRV_MEM_PCHECKS && !ctx) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }

        if(DRV_MEM_PCHECKS && !alloc_id) {
                assert(!"DRV_MEM_RESULT_BAD_PARAM");
                return DRV_MEM_RESULT_BAD_PARAM;
        }
        
        uint64_t idx = (alloc_id & 0xFFFFFFFF);
        uint64_t type = ((alloc_id >> 32) & 0xFFFFFFFF);
        
        if(type != DRV_MEM_ALLOC_TAGGED && type != DRV_MEM_ALLOC_VTAGGED) {
                assert(!"DRV_MEM_RESULT_FAIL");
                return DRV_MEM_RESULT_FAIL;
        }

        if(type == DRV_MEM_ALLOC_TAGGED) {
                if(DRV_MEM_PCHECKS && idx >= DRV_MEM_MAX_TAGGED_ALLOC) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
                /* clear tag */
                struct drv_tag_alloc *alloc = &ctx->alloc_tagged[idx];
                
                size_t i;
                void *addr = 0;

                for(i = 0; i < alloc->chunk_count; ++i) {
                        uint64_t id = alloc->alloc_ids[i];
                        if(id == tag_id) {
                                size_t chunk_start = alloc->chunk_bytes * i;
                                addr = &alloc->start[chunk_start];

                                alloc->alloc_ids[i] = 0;
                        }
                }

                return DRV_MEM_RESULT_OK;
        }

        return DRV_MEM_RESULT_FAIL;
}


/* ---------------------------------------------------------------- Config -- */


#undef DRV_MEM_PCHECKS
#undef DRV_MEM_MAX_STACK_ALLOC
#undef DRV_MEM_MAX_STACK_VALLOC
#undef DRV_MEM_MAX_TAGGED_ALLOC
#undef DRV_MEM_MAX_TAGGED_VALLOC
#undef DRV_MEM_ALLOC_STACK
#undef DRV_MEM_ALLOC_VSTACK
#undef DRV_MEM_ALLOC_TAGGED
#undef DRV_MEM_ALLOC_VTAGGED
