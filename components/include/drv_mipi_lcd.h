#ifndef _DRV_MIPI_LCD_H_
#define _DRV_MIPI_LCD_H_

#include "osi_compiler.h"
#include "fibocom.h"

OSI_EXTERN_C_BEGIN

/**
 * @brief mipi lcd result
 * 
 */
#define MIPI_LCD_RSL_SUCCESS           (0)   /* operation success */
#define MIPI_LCD_RSL_NOT_INIT          (-1)  /* mipi lcd driver has not benn inited yet */
#define MIPI_LCD_RSL_HAS_INITED        (-2)  /* mipi lcd driver has been inited before */
#define MIPI_LCD_RSL_FAIL_INIT         (-3)  /* mipi lcd driver fail to init */
#define MIPI_LCD_RSL_INVALID_PARAM     (-4)  /* invalid parameter */
#define MIPI_LCD_RSL_SLEEPING          (-5)  /* is sleeping */
#define MIPI_LCD_RSL_PARAM_OUT_RANGE   (-6)  /* parameter is out of range */
#define MIPI_LCD_RSL_NOT_SUPPORT       (-7)  /* operation is not supported */
#define MIPI_LCD_RSL_TIMEOUT           (-8)  /* operation timeout */


typedef struct {
    uint32_t frame0_over : 1;   // frame0 over
    uint32_t frame1_over : 1;   // frame1 over
    uint32_t frame2_over : 1;   // frame2 over
    uint32_t frame_running : 1; // is running or not
    uint32_t cur_frame : 2;     // current frame
} dpi_stat_t;

OSI_EXTERN_C_END
#endif