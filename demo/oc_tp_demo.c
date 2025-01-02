/**
 * @file oc_tp_demo.c
 * @author Guo Junhuang (Ethan Guo) (ethan.guo@fibocom.com)
 * @brief TP demo, including painter and touch events reported.
 * @version 0.1
 * @date 2022-11-10
 * 
 * @copyright Copyright (c) 2022
 * 
 * @note This demo can be used directly in AITI_Q390 machine.
 */
#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// #define LCD_PWR_EN_PIN        120
// #define LCD_PWR_EN_PIN_MODE   2

#define LCD_BL_EN_PIN        17
#define LCD_BL_EN_PIN_MODE   0

#define TP_X_ADC            3
#define TP_Y_ADC            1
#define TP_XP_PIN           29
#define TP_XN_PIN           32
#define TP_YP_PIN           53
#define TP_YN_PIN           27

#define TP_CALIB_COORD_X0  60
#define TP_CALIB_COORD_Y0  64
#define TP_CALIB_COORD_X1  180
#define TP_CALIB_COORD_Y1  64
#define TP_CALIB_COORD_X2  120
#define TP_CALIB_COORD_Y2  192

lcdDisplay_t bg_area = {
    .x = 0,
    .y = 0,
    .width = 0,
    .height = 0,
};

void tp_touch_cb(tp_result_t tp_result)
{
    tp_result_t new_tp_rsl = tp_result;
    int64_t newtime = 0;
    static int64_t oldtime = 0;
    static tp_result_t old_tp_rsl = {0};

    if (new_tp_rsl.event == TP_TOUCH_EVENT_press) {
        newtime = osiUpTime();
        
        if (new_tp_rsl.event == 1) {
            fibo_lcd_SetPixel(new_tp_rsl.coord.x, new_tp_rsl.coord.y, GREEN);
        }

        if ((newtime - oldtime) < 50) {
            fibo_lcd_DrawLine(new_tp_rsl.coord.x, new_tp_rsl.coord.y, old_tp_rsl.coord.x, old_tp_rsl.coord.y, GREEN);
        }

        old_tp_rsl.coord.x = new_tp_rsl.coord.x;
        old_tp_rsl.coord.y = new_tp_rsl.coord.y;
        old_tp_rsl.event = new_tp_rsl.event;
        oldtime = newtime;
    } else {
        OSI_LOGI(0, "TP event: (%u, %u) event: %d", new_tp_rsl.coord.x, new_tp_rsl.coord.y, new_tp_rsl.event);
    }
}

void tp_calib_cb(tp_calib_phase_t tp_calib_phase)
{
    switch (tp_calib_phase) {
        case TP_CALIB_PHASE_wait_1st:
        {
            fibo_lcd_FillRect(&bg_area, BLACK);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X0,     TP_CALIB_COORD_Y0,     GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X0 + 1, TP_CALIB_COORD_Y0,     GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X0,     TP_CALIB_COORD_Y0 + 1, GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X0 + 1, TP_CALIB_COORD_Y0 + 1, GREEN);
        }
        break;
        
        case TP_CALIB_PHASE_wait_2nd:
        {
            fibo_lcd_FillRect(&bg_area, BLACK);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X1,     TP_CALIB_COORD_Y1,     GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X1 + 1, TP_CALIB_COORD_Y1,     GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X1,     TP_CALIB_COORD_Y1 + 1, GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X1 + 1, TP_CALIB_COORD_Y1 + 1, GREEN);
        }
        break;

        
        case TP_CALIB_PHASE_wait_3th:
        {
            fibo_lcd_FillRect(&bg_area, BLACK);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X2,     TP_CALIB_COORD_Y2,     GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X2 + 1, TP_CALIB_COORD_Y2,     GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X2,     TP_CALIB_COORD_Y2 + 1, GREEN);
            fibo_lcd_SetPixel(TP_CALIB_COORD_X2 + 1, TP_CALIB_COORD_Y2 + 1, GREEN);
        }
        break;

        
        case TP_CALIB_PHASE_done:
        {
            fibo_lcd_FillRect(&bg_area, BLACK);
        }
        break;

        default:
        break;
    }
}


static void tpThreadEntry(void *param)
{
	int32_t ret = -1;
    uint32_t lcd_devid = 0;
    uint32_t lcd_width = 0;
    uint32_t lcd_height = 0;

    OSI_LOGI(0, "tpThreadEntry TP application thread enter, param 0x%x", param);

    // turn off lpg
	if (fibo_lpg_switch(false) == -1) {
        OSI_PRINTFI("tpThreadEntry error in fibo_lpg_switch");
	}

    fibo_pwmOpen();
    fibo_pwtConfig(1000, 199, 900);
    fibo_pwtStartorStop(true);

    fibo_hal_pmu_setlevel(0, 1);    // if LCD IOs' level is 3.2V
    ret = fibo_lcd_init();
	if (ret != 0) {
		OSI_LOGI(0, "tpThreadEntry fibo_lcd_init failed:%d.", ret);
	} else {
		OSI_LOGI(0, "tpThreadEntry fibo_lcd_init success.");
	}
	
    ret = fibo_lcd_Getinfo(&lcd_devid, &lcd_width, &lcd_height);
	if (ret != 0) {
		OSI_LOGI(0, "tpThreadEntry fibo_lcd_Getinfo failed:%d.", ret);
	} else {
		OSI_LOGI(0, "tpThreadEntry fibo_lcd_Getinfo success.");
	}
    OSI_PRINTFI("tpThreadEntry lcd devid: %d, lcd width: %d, lcd heigth: %d", lcd_devid, lcd_width, lcd_height);

    fibo_lcd_SetBrushDirection(LCD_DIRECT_NORMAL);

	bg_area.width = lcd_width;
 	bg_area.height = lcd_height;

    fibo_lcd_FillRect(&bg_area, BLACK);

    tp_cfg_t _g_tp_cfg = {
        .x_adc = TP_X_ADC,
        .y_adc = TP_Y_ADC,
        .xp_pin = TP_XP_PIN,
        .xn_pin = TP_XN_PIN,
        .yp_pin = TP_YP_PIN,
        .yn_pin = TP_YN_PIN,
    };
    tp_point_t calib_coords[3] = {{TP_CALIB_COORD_X0, TP_CALIB_COORD_Y0}, 
                                  {TP_CALIB_COORD_X1, TP_CALIB_COORD_Y1}, 
                                  {TP_CALIB_COORD_X2, TP_CALIB_COORD_Y2}};

    fibo_tp_init(_g_tp_cfg);
    fibo_tp_load_calib(true, calib_coords, tp_calib_cb);    // force to re-calibrate
    //fibo_tp_load_calib(false, calib_coords, tp_calib_cb); // calibrate only if found no calibrating before
    // fibo_tp_load_calib(false, NULL, tp_calib_cb);        // do not calibrate anyway
	OSI_LOGI(0, "tpThreadEntry fibo_tp_load_calib end");
    fibo_tp_open(tp_touch_cb);

    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(tpThreadEntry, "tpThreadEntry", 1024*500, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
