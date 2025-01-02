/**
 * @file drv_tp.h
 * @author Guo Junhuang (Ethan Guo) (ethan.guo@fibocom.com)
 * @brief TP driver
 * @version 0.1
 * @date 2021-04-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _DRV_TP_H_
#define _DRV_TP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "osi_compiler.h"

OSI_EXTERN_C_BEGIN

/**
 * @brief Enumeration of TP touch event
 */
typedef enum {
    TP_TOUCH_EVENT_none = 0,
    TP_TOUCH_EVENT_press,
    TP_TOUCH_EVENT_shortPress,
    TP_TOUCH_EVENT_longPress,
    TP_TOUCH_EVENT_shortRelease,
    TP_TOUCH_EVENT_longRelease,
} tp_touch_event_t;

/**
 * @brief Enumeration of TP calibration phase
 */
typedef enum {
    TP_CALIB_PHASE_wait_1st,
    TP_CALIB_PHASE_wait_2nd,
    TP_CALIB_PHASE_wait_3th,
    TP_CALIB_PHASE_done,
} tp_calib_phase_t;

/**
 * @brief Structure of TP point coordinate
 */
typedef struct
{
    int32_t x;
    int32_t y;
} tp_point_t;

/**
 * @brief Structure of TP configuration, including ADCs' and Pins' indexes
 */
typedef struct {
    // ADC index
    uint8_t x_adc;
    uint8_t y_adc;
    
    // pin index
    uint16_t xp_pin;
    uint16_t xn_pin;
    uint16_t yp_pin;
    uint16_t yn_pin;
} tp_cfg_t;

/**
 * @brief Structure of TP touch result, including coordinate and event
 */
typedef struct {
    tp_touch_event_t event;
    tp_point_t coord;
} tp_result_t;

/**
 * @brief TP tcouch callback
 */
typedef void (*tp_touch_cb_t)(tp_result_t);

/**
 * @brief TP calibration callback
 */
typedef void (*tp_calib_cb_t)(tp_calib_phase_t);

OSI_EXTERN_C_END

#endif
