#include <drive/app.h>
#include <stdlib.h>
#include <assert.h>


void *test_device();
void test_setup(struct drv_app_ctx *ctx);
void test_tick();
void test_shutdown();


int
main() {
        struct drv_app_ctx *app_ctx = nullptr;

        struct drv_plat_ctx_create_desc app_desc = {};
        app_desc.alloc_fn   = malloc;
        app_desc.free_fn    = free;
        app_desc.title      = "test";
        app_desc.width      = 100;
        app_desc.height     = 100;
        app_desc.gpu_device = test_device();

        drv_app_result app_ok = drv_app_ctx_create(&app_desc, &app_ctx);
        assert(app_ok == DRV_APP_RESULT_OK);
        
        test_setup(app_ctx);
        
        uint64_t evts = 0;
        while(drv_app_ctx_process(app_ctx, &evts) == DRV_APP_RESULT_OK) {
                test_tick();
        }
        
        test_shutdown();

        return 0;
}
