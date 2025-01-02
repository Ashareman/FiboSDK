/**
 * @file drv_genspi_lcd.h
 * @author Guo Junhuang (ethan.guo@fibocom.com)
 * @brief driver of genernal spi lcd
 * @version 0.1
 * @date 2022-07-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _DRV_GENSPI_LCD_H_
#define _DRV_GENSPI_LCD_H_

#include "drv_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GENSPI_LCD_DEV_NAME_LEN         (20)
#define GENSPI_LCD_DEV_DEFAULT_NAME     "defaultLcd"


/**
 * @brief Returning codes of general spi lcd APIs
 */
#define GENSPI_LCD_SUCCESS              (0)     /* operation success */
#define GENSPI_LCD_ERR_OPERATION        (-1)    /* operation failed */
#define GENSPI_LCD_ERR_NOT_INIT         (-2)    /* has not benn inited yet */
#define GENSPI_LCD_ERR_HAS_INITED       (-3)    /* has been inited before */
#define GENSPI_LCD_ERR_PARAM            (-4)    /* invalid parameter */
#define GENSPI_LCD_ERR_SLEEPING         (-5)    /* is sleeping */
#define GENSPI_LCD_ERR_NOT_SUPPORT      (-6)    /* operation is not supported */
#define GENSPI_LCD_ERR_TIMEOUT          (-7)    /* operation timeout */
#define GENSPI_LCD_ERR_MALLOC           (-8)    /* malloc error */
#define GENSPI_LCD_ERR_UNKNOWN_SPEC     (-9)    /* unknown lcd spec */
#define GENSPI_LCD_ERR_DEV_EMTPY        (-10)   /* device empty */

/**
 * @brief LCD SPI interface mode
 */
typedef enum {
    GENSPI_WIRE_UNKNOWN = 0,
    GENSPI_WIRE_4LINE_II,       // 4-line 8bit serial I/F II
    GENSPI_WIRE_3LINE_9BIT_II,  // 3-line 9bit serial I/F II
    GENSPI_WIRE_4LINE_I,        // 4-line 8bit serial I/F I (un-support read)
    GENSPI_WIRE_3LINE_9BIT_I,   // 3-line 9bit serial I/F I (un-support read)
    GENSPI_WIRE_MAX,
} genspi_lcd_wire_t;

/**
 * @brief Struction of genernal SPI LCD configuration
 */
typedef struct {

    /* SPI controller name: DRV_NAME_SPI1, DRV_NAME_SPI2, DRV_NAME_SPI3. */
    uint32_t spi_name;
    
    /* SPI writing transmission rate. */
    uint32_t spi_baud_w;

    /* SPI reading transmission rate. */
    uint32_t spi_baud_r;

    /* SPI CS pin select. */
    drvSpiCsSel cs_sel;

    /* Pin number used for LCD_CSX, which should be supported as GPIO function. Valid only if "cs" is SPI_GPIO_CS. */
    uint16_t csx_pin;               

    /* Pin number used for LCD_DCX, which should be supported as GPIO function. */ 
    uint16_t dcx_pin;               

    /* Pin number used for LCD_RESET, which should be supported as GPIO function. */
    uint16_t rst_pin;   

    /* LCD SPI interface mode. */
    genspi_lcd_wire_t wire_mode;    
} genspi_lcd_cfg_t;

#ifdef __cplusplus
}
#endif

#endif
