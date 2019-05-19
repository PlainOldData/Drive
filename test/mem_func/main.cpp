#include <drive/mem.h>
#include <assert.h>
#include <thread>
#include <cstdlib>
#include <cstdio>


/* ----------------------------------------------------------------- Tests -- */


int
stack_phy_test()
{
        drv_mem_ctx *ctx = nullptr;

        drv_mem_ctx_create_desc create_desc = {};
        create_desc.alloc_fn = malloc;
        create_desc.free_fn  = free;

        drv_mem_result mem_ok = drv_mem_ctx_create(&create_desc, &ctx);
        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert(!"Failed to create ctx");
                return 0;
        }

        size_t stack_size = 256;
        uint64_t alloc_id = 0;

        drv_mem_stack_allocator_desc alloc_desc = {};
        alloc_desc.alloc_type = DRV_MEM_ALLOC_TYPE_PHYSICAL;
        alloc_desc.size_of_stack = stack_size;

        mem_ok = drv_mem_stack_allocator_create(ctx, &alloc_desc, &alloc_id);

        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert(!"Failed to create allocator");
                return 0;
        }

        if(!alloc_id) {
                assert(!"Failed to return allocator id");
                return 0;
        }

        void *alloc = nullptr;
        mem_ok = drv_mem_allocate(ctx, 123, &alloc);

        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert(!"Failed to alloc memory");
                return 0;
        }

        mem_ok = drv_mem_free(ctx, alloc);

        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert("Failed to free memory");
                return 0;
        }

        mem_ok = drv_mem_ctx_destroy(&ctx);
        assert(ctx == nullptr);

        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert("Failed to free ctx");
                return 0;
        }

        return 1;
}


int
general_test() {
        drv_mem_ctx *ctx = nullptr;

        drv_mem_ctx_create_desc create_desc = {};
        create_desc.alloc_fn = malloc;
        create_desc.free_fn  = free;

        drv_mem_result mem_ok = drv_mem_ctx_create(&create_desc, &ctx);
        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert(!"Failed to create ctx");
                return 0;
        }

        void *alloc = nullptr;
        mem_ok = drv_mem_allocate(ctx, 123, &alloc);

        if(!alloc) {
                assert(!"No allocation");
                return 0;
        }

        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert(!"Failed to alloc memory");
                return 0;
        }

        mem_ok = drv_mem_free(ctx, alloc);

        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert("Failed to free memory");
                return 0;
        }

        mem_ok = drv_mem_ctx_destroy(&ctx);
        assert(ctx == nullptr);

        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert("Failed to free ctx");
                return 0;
        }

        return 1;
}


/* ------------------------------------------------------------ Test Entry -- */


#ifndef BATCH_COUNT
#define BATCH_COUNT 1
#endif


int
main() {

        /* test data */
        struct test_data {
                const char *name;
                decltype(general_test)* func;
        };
        
        test_data tests[] = {
                {"general_test", general_test},
                {"stack_phy_test", stack_phy_test},
        };
        
        
        
        /* run tests */
        for(int i = 0; i < BATCH_COUNT; ++i) {
                printf("Batch %d\n", i);
        
                for(auto &t : tests) {
                        int result = t.func();
                        
                        if(result != 1) {
                                fprintf(stderr, "Mem Test Failed %s", t.name);
                                return EXIT_FAILURE;
                        }
                }
        }
        
        return EXIT_SUCCESS;
}