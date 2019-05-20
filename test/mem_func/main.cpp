
#include <drive/mem.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* ----------------------------------------------------------------- Tests -- */


int
stack_virt_test()
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
        
        struct chunk_of_data {
                int id;
                char data[1048572];
        };

        size_t count = 512;
        size_t stack_size = sizeof(chunk_of_data) * count;
        uint64_t alloc_id = 0;

        drv_mem_stack_allocator_desc alloc_desc = {};
        alloc_desc.alloc_type    = DRV_MEM_ALLOC_TYPE_VIRTUAL;
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

        struct chunk_of_data *start;

        /* fill with data */
        for(size_t i = 0; i < count; ++i) {
                void *alloc = nullptr;
                
                size_t size = sizeof(chunk_of_data);
                mem_ok = drv_mem_stack_alloc(ctx, alloc_id, size, &alloc);
                
                if(mem_ok != DRV_MEM_RESULT_OK) {
                        assert(!"Failed to alloc memory");
                        return 0;
                }

                struct chunk_of_data *cod = (struct chunk_of_data*)alloc;
                cod->id = i;

                if(i == 0) {
                        start = cod;
                }
        }

        /* check */
        for(size_t i = 0; i < count; ++i) {
                assert(start[i].id == i);
        }
        
        mem_ok = drv_mem_stack_clear(ctx, alloc_id);
        
        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert("Failed to clear memory");
        }

        for(size_t i = 0; i < count; ++i) {
                void *alloc = nullptr;
                
                size_t size = sizeof(chunk_of_data);
                mem_ok = drv_mem_stack_alloc(ctx, alloc_id, size, &alloc);
                
                if(mem_ok != DRV_MEM_RESULT_OK) {
                        assert(!"Failed to alloc memory");
                        return 0;
                }

                struct chunk_of_data *cod = (struct chunk_of_data*)alloc;
                cod->id = i;
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
        
        const char *test_str1 = "Hello How Are You";
        const char *test_str2 = "Small Str";

        size_t stack_size = strlen(test_str1);
        uint64_t alloc_id = 0;

        drv_mem_stack_allocator_desc alloc_desc = {};
        alloc_desc.alloc_type    = DRV_MEM_ALLOC_TYPE_PHYSICAL;
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

        const char *test_buffer = 0;

        /* fill with data */
        for(size_t i = 0; i < strlen(test_str1); ++i) {
                void *alloc = nullptr;
                
                size_t size = 1;
                mem_ok = drv_mem_stack_alloc(ctx, alloc_id, size, &alloc);
                
                if(mem_ok != DRV_MEM_RESULT_OK) {
                        assert(!"Failed to alloc memory");
                        return 0;
                }
                
                /* save out start of stack */
                if(i == 0) {
                        test_buffer = (const char *)alloc;
                }
                
                char *alloc_str = (char*)alloc;
                *alloc_str = test_str1[i];
        }

        assert(strncmp(test_str1, test_buffer, strlen(test_str1)) == 0);
        
        mem_ok = drv_mem_stack_clear(ctx, alloc_id);
        
        if(mem_ok != DRV_MEM_RESULT_OK) {
                assert("Failed to clear memory");
        }

        /* fill with data again */
        for(size_t i = 0; i < strlen(test_str2); ++i) {
                void *alloc = nullptr;
                
                size_t size = 1;
                mem_ok = drv_mem_stack_alloc(ctx, alloc_id, size, &alloc);
                
                if(mem_ok != DRV_MEM_RESULT_OK) {
                        assert(!"Failed to alloc memory");
                        return 0;
                }
                
                /* save out start of stack */
                if(i == 0) {
                        test_buffer = (const char *)alloc;
                }
                
                char *alloc_str = (char*)alloc;
                *alloc_str = test_str2[i];
        }
        
        assert(strncmp(test_str2, test_buffer, strlen(test_str2)) == 0);

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
#define BATCH_COUNT 1 << 22
#endif


int
main() {

        /* test data */
        struct test_data {
                const char *name;
                decltype(general_test)* func;
        };
        
        test_data tests[] = {
                //{"general_test", general_test},
                //{"stack_phy_test", stack_phy_test},
                {"stack_virt_test", stack_virt_test},
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
