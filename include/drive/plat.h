#ifndef DRV_PLAT_INCLUDED_
#define DRV_PLAT_INCLUDED_


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* ----------------------------------------------------------- Identifiers -- */


typedef enum _drv_plat_result {
        DRV_PLAT_RESULT_OK,
        DRV_PLAT_RESULT_BAD_PARAMS,
        DRV_MEM_RESULT_INVALID_DESC,
} drv_plat_result;


typedef enum _drv_plat_event {
        DRV_PLAT_EVENT_WIN_MOVED = 1 << 0,
        DRV_PLAT_EVENT_WIN_CLOSED = 1 << 1,
        DRV_PLAT_EVENT_INPUT = 1 << 2,
        DRV_PLAT_EVENT_EXTERNAL = 1 << 3,
} drv_plat_event;


/* -------------------------------------------------------------- Lifetime -- */

/* -------------------------------------------------------------- Monitors -- */

/* ---------------------------------------------------------------- Window -- */


struct drv_plat_window_desc {
        uint32_t flags;
};


/* ----------------------------------------------------------------- Input -- */


#ifdef __cplusplus
} /* extern */
#endif


#endif