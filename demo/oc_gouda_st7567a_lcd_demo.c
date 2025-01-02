/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

//
extern INT32 fibo_lcd_rstPin_setLevel(SAPP_IO_ID_T id,UINT8 level);
extern INT32 fibo_lcd_spi_config(uint8_t spilinetype, uint32_t spiclk);
extern INT32 fibo_lcd_spi_set_freq(uint32_t spiclk);
extern INT32 fibo_lcd_spi_set_mode(uint8_t spilinetype);
extern INT32 fibo_lcd_spi_write_cmd(uint16_t cmd);
extern INT32 fibo_lcd_spi_write_data(uint16_t data);
extern INT32 fibo_lcd_spi_read_data(uint16_t addr, uint8_t * pData, uint32_t len);

#define LCD_WIDTH       132 // 96
#define LCD_HEIGHT      64

#define LCD_PAGE        8
#define LCD_LINE        64


static uint8_t g_st7567a_inst_regRes      = 0x06;     //  (0~7): Select Regulator Resister
static uint8_t g_st7567a_inst_EVReg       = 0x18;      //  (0~63): Set Electronic Volume Register
static uint8_t g_st7567a_inst_bias        = 0x00;       // (0~7): 0-1/5 1-1/6 2-1/7 3-1/8 4-1/9 5-1/10 6-1/11 7-1/12
static bool    g_st7567a_inst_isActive    = false;


#define INST_regRes_MASK            0x20
#define INST_EVReg_MASK             0x00
#define INST_bias_MASK              0x50
#define INST_STARTLINE_MASK         0x40

#define INST_regRes_VALID_BITS      0x07
#define INST_EVReg_VALID_BITS       0x3F
#define INST_bias_VALID_BITS        0x01//0x07
#define INST_STARTLINE_VALID_BITS   0x3F

//  (0~7): Select Regulator Resister
//  0-3.0; 1-3.5; 2-4.0; 3-4.5; 4-5; 5-5.5; 6-6; 7-7.5;
#define INST_regRes(x)    (INST_regRes_MASK | ((x) & INST_regRes_VALID_BITS))
//  (0~63): Set Electronic Volume Register
#define INST_EVReg(x)     (INST_EVReg_MASK | ((x) & INST_EVReg_VALID_BITS))
// (0~1): 0-1/9 1-1/7 (at 1/65 duty)
#define INST_bias(x)      (INST_bias_MASK | ((x) & INST_bias_VALID_BITS))
//(0~63): set start line
#define INST_startline(x)    (INST_STARTLINE_MASK | ((x) & INST_STARTLINE_VALID_BITS))

#define PAGE_LINE_NUM   8

#define WHICH_PAGE(y)               ((y) / PAGE_LINE_NUM)
#define WHICH_BIT_IN_PAGE(y)        ((y) % PAGE_LINE_NUM)
#define WHICH_COLUMN(x)             (x)

#define ST7567A_GOUDA_SPI_LINE_TYPE        HAL_GOUDA_SPI_LINE_4   // 4 line spi
#define ST7567A_GOUDA_SPI_CLK              5000000                // Serial Clock Period Min.: 200ns


#define LCD_DataWrite_ST(Data)                    \
    {                                                 \
        while (fibo_lcd_spi_write_data(Data) != 0) \
            ;                                         \
    }
#define LCD_CtrlWrite_ST(Cmd)                   \
    {                                               \
        while (fibo_lcd_spi_write_cmd(Cmd) != 0) \
            ;                                       \
    }


static void st7567a_set_addr(uint8_t column, uint8_t page)
{
    column = column;
    LCD_CtrlWrite_ST(0xb0 + page);
    LCD_CtrlWrite_ST(((column >> 4) & 0x07) + 0x10);
    LCD_CtrlWrite_ST(column & 0x0f);
}

static void st7567a_clean_screen()
{
    for (uint8_t j = 0; j < LCD_PAGE; j++) {
        st7567a_set_addr(0, j);
        for (uint8_t i = 0; i < LCD_WIDTH; i++) {
            LCD_DataWrite_ST(0x00);
        }
    }
}

static void st7567a_spi_init()
{
    fibo_lcd_spi_config(ST7567A_GOUDA_SPI_LINE_TYPE, ST7567A_GOUDA_SPI_CLK);
}

static void st7567a_RSTB_set(bool high)
{
    if(high)
    {
        fibo_lcd_rstPin_setLevel(71,0);
    }
    else
    {
        fibo_lcd_rstPin_setLevel(71,1);
    }
}

static void st7567a_power_set(bool is_3200_mv)
{
    if(is_3200_mv) //3.2V
    {
        fibo_hal_pmu_setlevel(0, 1);
    }
    else //1.8V
    {
        fibo_hal_pmu_setlevel(0, 1);
    }

}

static void st7567a_backlight_set(bool open)
{
    //open backligit
    if(open)
    {
        fibo_sink_OnOff(0,true);
        fibo_sinkLevel_Set(0,50);
    }
    else
    {
        fibo_sink_OnOff(0,false);
    }
}

static void st7567a_write_display_date(uint8_t page, uint8_t column, unsigned char *data, int size)
{
    if(g_st7567a_inst_isActive)
    {
        st7567a_set_addr(column, page);
        for(int i = 0; i < size; i++)
        {
            LCD_DataWrite_ST(*(data + i));
        }

        LCD_CtrlWrite_ST(0xAF);
    }
}

static void st7567a_open()
{
    OSI_LOGI(0, "lcd[st7567a]:   st7567a_open ");

    fibo_taskSleep(100); // Delay 100ms for stable VDD1/VDD2/VDD3

    //HWrest
    st7567a_RSTB_set(0);
    fibo_taskSleep(5);
    st7567a_RSTB_set(1);
    fibo_taskSleep(5);

    //set parameters
    LCD_CtrlWrite_ST(0xE2);
    fibo_taskSleep(10);
    LCD_CtrlWrite_ST(0x2C);
    fibo_taskSleep(10);
    LCD_CtrlWrite_ST(0x2E);
    fibo_taskSleep(10);
    LCD_CtrlWrite_ST(0x2F);
    fibo_taskSleep(10);

    LCD_CtrlWrite_ST(0x24);
    LCD_CtrlWrite_ST(0x81);
    LCD_CtrlWrite_ST(0x15);

    LCD_CtrlWrite_ST(0xA0); // set SEG Direction
    LCD_CtrlWrite_ST(0xC8); // set COM Direction

    LCD_CtrlWrite_ST(0x40); // set start line

    LCD_CtrlWrite_ST(0xAF); // Display ON

    OSI_LOGI(0, "lcd[st7567a]:   st7567a_open done. ");
    g_st7567a_inst_isActive = true;

}

static void st7567a_set_display_param(uint8_t contrastRatio_coarse, uint8_t contrastRatio_fine, uint8_t bias)
{
 
	g_st7567a_inst_regRes = contrastRatio_coarse;
	g_st7567a_inst_EVReg = contrastRatio_fine;
	g_st7567a_inst_bias = bias;
	OSI_LOGI(0, "lcd[st7567a]:   set lcd dispaly param. ");

	if (g_st7567a_inst_isActive) {
		LCD_CtrlWrite_ST(INST_regRes(g_st7567a_inst_regRes)); // Select regulator register(1+(Ra+Rb))
		LCD_CtrlWrite_ST(0x81); // Set Reference Voltage 
		LCD_CtrlWrite_ST(INST_EVReg(g_st7567a_inst_EVReg)); // EV=35 => Vop =10.556V
		LCD_CtrlWrite_ST(INST_bias(g_st7567a_inst_bias)); // Set LCD Bias=1/9 V0
	}
}
static void st7567a_close()
{
    //HWrest
    st7567a_RSTB_set(0);
    fibo_taskSleep(5);
    st7567a_RSTB_set(1);
    fibo_taskSleep(5);
    g_st7567a_inst_isActive = false;
}

static void st7567a_power_save_mode_enter()
{
    /*Enter The Power Save Mode*/
    if(g_st7567a_inst_isActive)
    {
        LCD_CtrlWrite_ST(0xAE); //display off
        LCD_CtrlWrite_ST(0xA5); //All Pixel ON
    }
}

static void st7567a_power_save_mode_exit()
{
    /*Exiting The Power Save Mode*/
    if(g_st7567a_inst_isActive)
    {
        LCD_CtrlWrite_ST(0xA4); //Cancel All Pixel ON
        LCD_CtrlWrite_ST(0xAF); // Display ON
    }
}

extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

unsigned char picture[8][132] = {
{0x00,0x00,0x44,0x7e,0x7e,0x40,0x00,0x00,0x00,0x00,0x44,0x7e,0x7e,0x40,0x00,0x00,0x00,0x00,0x44,0x7e,0x7e,0x40,0x00,0x00,0x00,0x00,0x44,0x7e,0x7e,0x40,0x00,0x00},
{0xFF,0x00,0x76,0x56,0x56,0x5e,0x00,0x00,0xFF,0x00,0x76,0x56,0x56,0x5e,0x00,0x00,0xFF,0x00,0x76,0x56,0x56,0x5e,0x00,0x00,0xFF,0x00,0x76,0x56,0x56,0x5e,0x00,0x00},
{0x00,0x00,0x52,0x52,0x52,0x7e,0x00,0x00,0x00,0x00,0x52,0x52,0x52,0x7e,0x00,0x00,0x00,0x00,0x52,0x52,0x52,0x7e,0x00,0x00,0x00,0x00,0x52,0x52,0x52,0x7e,0x00,0x00},
{0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c},
{0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c},
{0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c},
{0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c},
{0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x01,0x02,0x04,0x08,0x09,0x0a,0x0b,0x0c}
};

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
	st7567a_spi_init();
    st7567a_power_set(1);
    st7567a_backlight_set(1);
    st7567a_open();
    st7567a_set_display_param(0x06,0x18,0);

    while(1)
    {
        for(int i = 0; i < 8; i ++)
        {
            st7567a_write_display_date(i, 0, picture[i], sizeof(picture[i]));
        }
        OSI_LOGI(0, "LCD test!");
        fibo_taskSleep(1000*5);
    }
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*16, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
