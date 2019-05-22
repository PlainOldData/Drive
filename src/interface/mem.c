

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
        LPVOID *start;
        DWORD page_size;
        SIZE_T pages_reserved;
        SIZE_T bytes;
        SIZE_T curr;
        #else
        #error "No Impl"
        #endif
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
                        if(ctx->valloc_stack[i].start) {
                                BOOL ok = VirtualFree(
                                        ctx->valloc_stack[i].start,
                                        0,
                                        MEM_RELEASE);
                                
                                if (!ok) {
                                        DWORD err = GetLastError();
                                        assert(!"Failed in release");
                                }
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

        if(bytes) {
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

                BOOL ok = VirtualFree(alloc->start, 0, MEM_DECOMMIT);
                alloc->curr = 0;
                alloc->pages_reserved = 0;

                if(!ok) {
                        assert(!"DRV_MEM_RESULT_FAIL");
                        return DRV_MEM_RESULT_FAIL;
                }
                
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
        size_t bytes)
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
#undef DRV_MEM_ALLOC_STACK
#undef DRV_MEM_ALLOC_VSTACK
#undef DRV_MEM_ALLOC_TAGGED
#undef DRV_MEM_ALLOC_VTAGGED
