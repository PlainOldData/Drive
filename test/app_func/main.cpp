#include <drive/app.h>
#include <stdlib.h>
#include <assert.h>
#include <test.h>


int
main() {
        drv_app_result app_ok = DRV_APP_RESULT_OK;

        struct drv_app_ctx *app_ctx = nullptr;
        void *gpu_device = nullptr;
        
        #if __APPLE__
        app_ok = drv_app_gpu_device(DRV_APP_GPU_DEVICE_METAL, &gpu_device);
        assert(app_ok == DRV_APP_RESULT_OK);
        #endif

        struct drv_app_ctx_create_desc app_desc = {};
        app_desc.alloc_fn   = malloc;
        app_desc.free_fn    = free;
        app_desc.width      = 1200;
        app_desc.height     = 720;
        app_desc.gpu_device = gpu_device;

        app_ok = drv_app_ctx_create(&app_desc, &app_ctx);
        assert(app_ok == DRV_APP_RESULT_OK);
        
        test_setup(app_ctx);
        
        uint64_t evts = 0;
        while(drv_app_ctx_process(app_ctx, &evts) == DRV_APP_RESULT_OK) {
                test_tick();
        }
        
        test_shutdown();

        return 0;
}
