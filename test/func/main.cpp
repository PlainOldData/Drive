#include <drive/sched.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


void
drv_logging(const char *msg) {
        printf("DRV SCHED: %s\n", msg);
}


/* ----------------------------------------------------------------- Tests -- */
/*
 * Batch with waits 
 */
int
batch_and_wait()
{
        struct drv_sched_ctx *drv = nullptr;
        drv_sched_result success = DRV_SCHED_RESULT_OK;
        
        /* create ctx */
        struct drv_sched_ctx_create_desc ctx_create_desc = {};
        ctx_create_desc.thread_count  = 0;
        ctx_create_desc.thread_pin    = 0;
        ctx_create_desc.thread_name   = "DRV_WORK";
        ctx_create_desc.sched_log     = nullptr;
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.sched_log     = drv_logging;
        
        success = drv_sched_ctx_create(&ctx_create_desc, &drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to create ctx");
                return 0;
        }
        
        /* issue some work */
        constexpr int batch_size = 16;
        int test_value[batch_size] = {};
        struct drv_sched_work_desc work_desc[batch_size] = {};

        for(int i = 0; i < batch_size; ++i) {
                work_desc[i].arg = &test_value[i];
                work_desc[i].tid_lock = 0;
                work_desc[i].func = [](struct drv_sched_ctx*ctx, void *arg){
                        constexpr int sub_batch_size = 32;
                        int vals[sub_batch_size] = {};
                        struct drv_sched_work_desc wk[sub_batch_size] = {};

                        for(int i = 0; i < sub_batch_size; ++i) {
                                wk[i].arg = &vals[i];
                                wk[i].tid_lock = 0;
                                wk[i].func = [](struct drv_sched_ctx*ctx, void *arg) {
                                        (void)ctx;
                                        int*v = (int*)arg;
                                        *v = 1;
                                };
                        }

                        struct drv_sched_enqueue_desc enq_desc = {};
                        enq_desc.work = wk;
                        enq_desc.work_count = sub_batch_size;
                        enq_desc.blocked_mk = 0;

                        uint64_t mk = 0;
                        drv_sched_result success = DRV_SCHED_RESULT_OK;
                        success = drv_sched_enqueue(ctx, &enq_desc, &mk);
                        
                        if(success != DRV_SCHED_RESULT_OK) {
                                assert(!"Failed to enqueue");
                        }

                        success = drv_sched_wait(ctx, mk);
                        
                        if(success != DRV_SCHED_RESULT_OK) {
                                assert(!"Failed to wait");
                        }
                        
                        int *v = (int*)arg;
                        
                        for(int i = 0; i < sub_batch_size; ++i) {
                                *v += vals[i];
                                assert(vals[i]);
                        }
                };
        }
        
        /* submit some work */
        struct drv_sched_enqueue_desc enq_desc = {};
        enq_desc.work = work_desc;
        enq_desc.work_count = batch_size;
        enq_desc.blocked_mk = 0;

        uint64_t mk = 0;
        success = drv_sched_enqueue(drv, &enq_desc, &mk);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to enqueue work");
                return 0;
        }
        
        success = drv_sched_ctx_join(drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to wait");
                return 0;
        }
        
        /* test values */
        for(auto i : test_value) {
                if(i != 123) {
                        assert(!"Failed to execute work");
                        return 0;
                }
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

/*
 * Batch and Join
 * --------------
 * Submit a batch of work and have main thread join in.
 */

int
batch_and_join()
{
        struct drv_sched_ctx *drv = nullptr;
        drv_sched_result success = DRV_SCHED_RESULT_OK;
        
        /* create ctx */
        struct drv_sched_ctx_create_desc ctx_create_desc = {};
        ctx_create_desc.thread_count  = 0;
        ctx_create_desc.thread_pin    = 0;
        ctx_create_desc.thread_name   = "DRV_WORK";
        ctx_create_desc.sched_log     = nullptr;
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.sched_log     = drv_logging;
        
        success = drv_sched_ctx_create(&ctx_create_desc, &drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to create ctx");
                return 0;
        }
        
        /* issue some work */
        constexpr int batch_size = 16;
        int test_value[batch_size] = {};
        struct drv_sched_work_desc work_desc[batch_size] = {};

        for(int i = 0; i < batch_size; ++i) {
                work_desc[i].arg = &test_value[i];
                work_desc[i].tid_lock = 0;
                work_desc[i].func = [](struct drv_sched_ctx*, void *arg){
                        int *argi = static_cast<int*>(arg);
                        *argi = 123;
                };
        }
        
        /* submit some work */
        struct drv_sched_enqueue_desc enq_desc = {};
        enq_desc.work = work_desc;
        enq_desc.work_count = batch_size;
        enq_desc.blocked_mk = 0;

        uint64_t mk = 0;
        success = drv_sched_enqueue(drv, &enq_desc, &mk);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to enqueue work");
                return 0;
        }
        
        success = drv_sched_ctx_join(drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to wait");
                return 0;
        }
        
        /* test values */
        for(auto i : test_value) {
                if(i != 123) {
                        assert(!"Failed to execute work");
                        return 0;
                }
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
        ctx_create_desc.thread_count  = 0;
        ctx_create_desc.thread_pin    = 0;
        ctx_create_desc.thread_name   = "DRV_WORK";
        ctx_create_desc.sched_log     = nullptr;
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.sched_log     = drv_logging;
        
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
        
        success = drv_sched_wait(drv, mk);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to wait");
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
                {"batch_and_join", batch_and_join},
                {"batch_and_wait", batch_and_wait},
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
