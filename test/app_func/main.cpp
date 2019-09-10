#include <drive/app.h>
#include <stdlib.h>
#include <assert.h>
#include <test.h>


int
main() {
        /* -- GPU setup -- */

        drv_app_result app_ok = DRV_APP_RESULT_OK;

        struct drv_app_ctx app_ctx = {};
        struct drv_app_gpu_device app_dev = {};

        #if defined(__APPLE__)
        auto gpu_device_type = DRV_APP_GPU_DEVICE_METAL;
        #elif defined(_WIN32) 
        auto gpu_device_type = DRV_APP_GPU_DEVICE_DX12;
        #endif

        app_ok = drv_app_gpu_device_create(gpu_device_type, &app_dev);
        assert(app_ok == DRV_APP_RESULT_OK);

        /* -- App setup -- */

        struct drv_app_ctx_create_desc app_desc = {};
        app_desc.width      = 1200;
        app_desc.height     = 720;
        app_desc.gpu_device = &app_dev;

        app_ok = drv_app_ctx_create(&app_desc, &app_ctx);
        assert(app_ok == DRV_APP_RESULT_OK);
        
        /* -- Input setup -- */

        uint8_t *keys = 0;
        app_ok = drv_app_input_kb_data_get(&app_ctx, &keys);
        assert(app_ok == DRV_APP_RESULT_OK);
        assert(keys);

        uint8_t *quit_key = &keys[DRV_APP_KB_ESC];
        assert(quit_key);

        /* -- App -- */

        test_setup(&app_ctx);

        uint64_t evts = 0;
        while(drv_app_ctx_process(&app_ctx, &evts) == DRV_APP_RESULT_OK) {
                
                if(evts & DRV_APP_EVENT_INPUT) {
                        if(*quit_key & DRV_APP_BUTTON_STATE_UP_EVENT) {
                                break;
                        }
                }

                test_tick();
        }
        
        test_shutdown();

        return 0;
}
