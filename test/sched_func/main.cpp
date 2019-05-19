#include <drive/sched.h>
#include <assert.h>
#include <thread>
#include <cstdlib>
#include <cstdio>


void
drv_logging(drv_sched_log_type type, const char *msg, void *ud) {
        (void)ud;

        static const char *type_str[] = {"Error", "Warning", "Info"};
        assert(type <= DRV_SCHED_LOG_INFO);
        
        printf("Sched - %s: %s\n", type_str[type], msg);
}

/* ----------------------------------------------------------------- Tests -- */
/*
 *  Deep test
 *  ---------
 *  This test tries to exersize the code with all senarios.
 */
void
logic(drv_sched_ctx *ctx, void *arg) {
        constexpr int log_count = 256;
        drv_sched_result success = DRV_SCHED_RESULT_OK;
        
        /* ensure last batch has finished */
        static uint64_t last_marker = 0;
        drv_sched_wait(ctx, last_marker);
        
        struct drv_sched_work_desc logic_work[log_count];
        
        for(int i = 0; i < log_count; ++i) {
                logic_work[i].func = [](drv_sched_ctx *, void *) {
                };
                logic_work[i].arg = arg;
                logic_work[i].tid_lock = 0;
        }
        
        struct drv_sched_enqueue_desc enqueue = {};
        enqueue.blocked_mk = 0;
        enqueue.work = &logic_work[0];
        enqueue.work_count = log_count;
        
        /* fire and forget */
        success = drv_sched_enqueue(ctx, &enqueue, &last_marker);
        assert(success == DRV_SCHED_RESULT_OK);
        
        /* test result */
        int *int_arg = (int*)arg;
        assert(*int_arg == 0);
        *int_arg += 1;
};

void
physics(drv_sched_ctx *ctx, void *arg) {
        (void)ctx;
        
        int *int_arg = (int*)arg;
        assert(*int_arg == 1);
        *int_arg += 1;
};

void
render(drv_sched_ctx *ctx, void *arg) {
        (void)ctx;
        
        int *int_arg = (int*)arg;
        assert(*int_arg == 2);
        *int_arg += 1;
};

void
shutdown(drv_sched_ctx *ctx, void *arg) {
        (void)ctx;
        
        int *int_arg = (int*)arg;
        assert(*int_arg == 1024);
        
};

void frame(drv_sched_ctx *ctx, void *arg) {
        drv_sched_result success;

        int *frame_counter = (int*)arg;
        
        int counter = 0;
        
        /* create desciptions for frame */
        struct drv_sched_work_desc log_frame_desc = {};
        log_frame_desc.arg      = &counter;
        log_frame_desc.func     = logic;
        log_frame_desc.tid_lock = 0;
        
        struct drv_sched_enqueue_desc log_enq_desc = {};
        log_enq_desc.work = &log_frame_desc;
        log_enq_desc.work_count = 1;
        log_enq_desc.blocked_mk = 0;
        
        uint64_t log_mk = 0;
        success = drv_sched_enqueue(ctx, &log_enq_desc, &log_mk);
        assert(success == DRV_SCHED_RESULT_OK);
        
        /* -- */
        
        struct drv_sched_work_desc phy_frame_desc = {};
        phy_frame_desc.arg      = &counter;
        phy_frame_desc.func     = physics;
        phy_frame_desc.tid_lock = 0;
        
        struct drv_sched_enqueue_desc phy_enq_desc = {};
        phy_enq_desc.work = &phy_frame_desc;
        phy_enq_desc.work_count = 1;
        phy_enq_desc.blocked_mk = log_mk;
        
        uint64_t phy_mk = 0;
        success = drv_sched_enqueue(ctx, &phy_enq_desc, &phy_mk);
        assert(success == DRV_SCHED_RESULT_OK);
        
        /* -- */
        
        struct drv_sched_work_desc rdr_frame_desc = {};
        rdr_frame_desc.arg      = &counter;
        rdr_frame_desc.func     = render;
        rdr_frame_desc.tid_lock = 1;
        
        struct drv_sched_enqueue_desc rdr_enq_desc = {};
        rdr_enq_desc.work = &rdr_frame_desc;
        rdr_enq_desc.work_count = 1;
        rdr_enq_desc.blocked_mk = phy_mk;

        uint64_t rdr_mk = 0;
        success = drv_sched_enqueue(ctx, &rdr_enq_desc, &rdr_mk);
        assert(success == DRV_SCHED_RESULT_OK);
        
        /* -- */
        
        success = drv_sched_wait(ctx, rdr_mk);
        assert(success == DRV_SCHED_RESULT_OK);
        assert(counter == 3);
        
        /* enqueue next frame */
        *frame_counter += 1;
        
        struct drv_sched_work_desc frame_desc = {};
        frame_desc.arg      = arg;
        frame_desc.func     = *frame_counter < 1024 ? frame : shutdown;
        frame_desc.tid_lock = 0;
        
        struct drv_sched_enqueue_desc enq_desc = {};
        enq_desc.work       = &frame_desc;
        enq_desc.work_count = 1;
        enq_desc.blocked_mk = 0;

        success = drv_sched_enqueue(ctx, &enq_desc, nullptr);
        assert(success == DRV_SCHED_RESULT_OK);
};

void startup(drv_sched_ctx *ctx, void *arg) {
        struct drv_sched_work_desc frame_desc = {};
        frame_desc.arg      = arg;
        frame_desc.func     = frame;
        frame_desc.tid_lock = 0;
        
        struct drv_sched_enqueue_desc enq_desc = {};
        enq_desc.work = &frame_desc;
        enq_desc.work_count = 1;
        enq_desc.blocked_mk = 0;

        drv_sched_result success = drv_sched_enqueue(ctx, &enq_desc, nullptr);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to enqueue work");
        }
};

int
deep_test() {
        struct drv_sched_ctx *drv = nullptr;
        drv_sched_result success = DRV_SCHED_RESULT_OK;
        
        /* create ctx */
        struct drv_sched_ctx_create_desc ctx_create_desc = {};
        ctx_create_desc.thread_count  = 0;
        ctx_create_desc.thread_pin    = 0;
        ctx_create_desc.thread_name   = "Drive Deep Test Worker";
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.log_fn        = drv_logging;
        
        success = drv_sched_ctx_create(&ctx_create_desc, &drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to create ctx");
                return 0;
        }
        
        int frame_counter = 0;
        
        struct drv_sched_work_desc work_desc = {};
        work_desc.func     = startup;
        work_desc.arg      = &frame_counter;
        work_desc.tid_lock = 0;
        
        /* submit some work */
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
        
        success = drv_sched_ctx_join(drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to wait");
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


/*
 *  Tid Test
 *  --------
 *  Test that tasks are thread locked
 */
int
tid_locked()
{
        struct drv_sched_ctx *drv = nullptr;
        drv_sched_result success = DRV_SCHED_RESULT_OK;
        
        /* create ctx */
        struct drv_sched_ctx_create_desc ctx_create_desc = {};
        ctx_create_desc.thread_count  = 0;
        ctx_create_desc.thread_pin    = 0;
        ctx_create_desc.thread_name   = "Drive TID Test Worker";
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.log_fn        = drv_logging;
        
        success = drv_sched_ctx_create(&ctx_create_desc, &drv);
        
        if(success != DRV_SCHED_RESULT_OK) {
                assert(!"Failed to create ctx");
                return 0;
        }
        
        /* issue some work */
        constexpr int batch_size = 8;
        std::thread::id test_value[batch_size] = {};
        std::fill_n(test_value, batch_size, std::this_thread::get_id());
        
        struct drv_sched_work_desc work_desc[batch_size] = {};

        for(int i = 0; i < batch_size; ++i) {
                work_desc[i].arg = &test_value[i];
                work_desc[i].tid_lock = 1;
                work_desc[i].func = [](drv_sched_ctx*ctx, void *arg) {
                        std::thread::id *id = (std::thread::id*)arg;
                        assert(*id == std::this_thread::get_id());
                
                        constexpr int sub_batch_size = 8;
                        
                        std::thread::id vals[sub_batch_size] = {};
                        std::fill_n(
                                vals,
                                sub_batch_size,
                                std::this_thread::get_id());
                        
                        struct drv_sched_work_desc wk[sub_batch_size] = {};
                        
                        for(int i = 0; i < sub_batch_size; ++i) {
                                wk[i].arg = &vals[i];
                                wk[i].tid_lock = 1;
                                wk[i].func = [](drv_sched_ctx*ctx, void *arg) {
                                        (void)ctx;
                                        
                                        using namespace std;
                                        
                                        thread::id *id = (thread::id*)arg;
                                        assert(*id == this_thread::get_id());
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
        ctx_create_desc.thread_name   = "Drive Batch and Wait Test Worker";
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.log_fn        = drv_logging;
        
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
                work_desc[i].func = [](drv_sched_ctx*ctx, void *arg){
                        constexpr int sub_batch_size = 32;
                        int vals[sub_batch_size] = {};
                        struct drv_sched_work_desc wk[sub_batch_size] = {};

                        for(int i = 0; i < sub_batch_size; ++i) {
                                wk[i].arg = &vals[i];
                                wk[i].tid_lock = 0;
                                wk[i].func = [](drv_sched_ctx*ctx, void *arg) {
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
                if(i != 32) {
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
        ctx_create_desc.thread_name   = "Drive Batch Join Test Worker";
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.log_fn        = drv_logging;
        
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
        ctx_create_desc.thread_name   = "Drive Simple Test Worker";
        ctx_create_desc.sched_alloc   = malloc;
        ctx_create_desc.log_fn        = drv_logging;
        
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


/* ------------------------------------------------------------ Test Entry -- */


#ifndef BATCH_COUNT
#define BATCH_COUNT 1
#endif


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
                {"tid_locked", tid_locked},
                {"deep_test", deep_test},
        };
        
        
        
        /* run tests */
        for(int i = 0; i < BATCH_COUNT; ++i) {
                printf("Batch %d\n", i);
        
                for(auto &t : tests) {
                        int result = t.func();
                        
                        if(result != 1) {
                                fprintf(stderr, "Sched Test Failed %s", t.name);
                                return EXIT_FAILURE;
                        }
                }
        }
        
        return EXIT_SUCCESS;
}
