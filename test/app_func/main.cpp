#include <drive/app.h>
#include <stdlib.h>
#include <Windows.h>


int
main() {
        struct drv_app_ctx *app_ctx = nullptr;

        struct drv_plat_ctx_create_desc app_desc = {};
        app_desc.alloc_fn = malloc;
        app_desc.free_fn  = free;
        app_desc.title    = "test";
        app_desc.width    = 100;
        app_desc.height   = 100;

        drv_app_result app_ok = drv_app_ctx_create(&app_desc, &app_ctx);

        uint64_t evts = 0;
        while(drv_app_ctx_process(app_ctx, &evts) == DRV_APP_RESULT_OK) {

        }

        return 0;
}