#include <drive/sched.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


/* ----------------------------------------------------------------- Tests -- */

/*
 * Simple
 * ------
 * Creates a drv_sched context, submits a single job destroys the context.
 */

int
simple()
{
        struct drv_sched_ctx *drv = nullptr;
        drv_sched_result success = DRV_SCHED_RESULT_OK;
        
        /* create ctx */
        struct drv_sched_ctx_create_desc ctx_create_desc = {};
        ctx_create_desc.thread_count = 0;
        ctx_create_desc.thread_pin   = 0;
        ctx_create_desc.thread_names = 0;
        ctx_create_desc.sched_log     = nullptr;
        ctx_create_desc.sched_alloc   = malloc;
        
        success = drv_sched_ctx_create(&ctx_create_desc, &drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to create ctx");
                return 0;
        }
        
        /* issue some work */
        int test_value = 0;
        
        struct drv_sched_work_desc work_desc = {};
        work_desc.arg = &test_value;
        work_desc.tid_lock = 0;
        work_desc.func = [](struct drv_sched_ctx*, void *arg){
                int *argi = static_cast<int*>(arg);
                *argi = 123;
        };
        
        struct drv_sched_enqueue_desc enq_desc = {};
        enq_desc.work = &work_desc;
        enq_desc.work_count = 1;
        enq_desc.blocked_mk = 0;
        
        uint64_t mk = 0;
        success = drv_sched_enqueue(drv, &enq_desc, &mk);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to enqueue work");
                return 0;
        }
        
        if(test_value != 123) {
                assert(!"Failed to execute work");
                return 0;
        }
        
        /* clean up */
        struct drv_sched_ctx_destroy_desc ctx_destroy_desc = {};
        ctx_destroy_desc.sched_free = free;
        ctx_destroy_desc.ctx_to_destroy = &drv;
        
        success = drv_sched_ctx_destroy(&ctx_destroy_desc);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to destroy ctx");
                return 0;
        }
        
        if(drv != nullptr) {
                assert(!"Failed to clear ctx");
                return 0;
        }
        
        return 1;
}


int
main()
{
        /* test data */
        struct test_data {
                const char *name;
                decltype(simple)* func;
        };
        
        test_data tests[] = {
                {"simple", simple},
        };
        
        /* run tests */
        for(auto &t : tests) {
                int result = t.func();
                
                if(result != 1) {
                        fprintf(stderr, "Test Failed %s", t.name);
                        return EXIT_FAILURE;
                }
        }
        
        return EXIT_SUCCESS;
}
