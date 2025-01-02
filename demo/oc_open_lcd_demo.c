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

extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

#define LCD_ID_ST7789v2       0X858552
#define LCD_WIDTH  240
#define LCD_HEIGHT 320


#define LCD_DataWrite_ST(Data)                    \
    {                                                 \
        while (fibo_lcd_spi_write_data(Data) != DRV_LCD_SUCCESS) \
            ;                                         \
    }
#define LCD_CtrlWrite_ST(Cmd)                   \
    {                                               \
        while (fibo_lcd_spi_write_cmd(Cmd) != DRV_LCD_SUCCESS) \
            ;                                       \
    }

static void _lcdDelayMs(int ms_delay)
{
    osiDelayUS(ms_delay * 1000);
}

OSI_UNUSED static void _stSetDirection(lcdDirect_t direct_type)
{

    OSI_LOGI(0, "lcd[st7789v2]:    _stSetDirection = %d", direct_type);

    switch (direct_type)
    {
    case LCD_DIRECT_NORMAL:
        //Vertical screen  startpoint at topleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0x00);
        break;
    case LCD_DIRECT_ROT_90:
        //Horizontal screen  startpoint at topleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0x60);
        break;
    case LCD_DIRECT_ROT_180:
        //Vertical screen  startpoint at topleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0xC0);
        break;
	case LCD_DIRECT_ROT_270:
        //Horizontal screen  startpoint at topleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0xA0);
        break;
    case LCD_DIRECT_NORMAL2:
        //Vertical screen  startpoint at bottomleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0x80);
        break;
    case LCD_DIRECT_NORMAL2_ROT_90:
        //Horizontal screen  startpoint at bottomleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0x20);
        break;
    case LCD_DIRECT_NORMAL2_ROT_180:
        //Vertical screen  startpoint at bottomleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0x40);
        break;
	case LCD_DIRECT_NORMAL2_ROT_270:
        //Horizontal screen  startpoint at topleft
        LCD_CtrlWrite_ST(0x36);
        LCD_DataWrite_ST(0xE0);
        break;
    }

    LCD_CtrlWrite_ST(0x2c);
}

/******************************************************************************/
//  Description:   Set the windows address to display, in this windows
//                 color is  refreshed.
/******************************************************************************/
static void _stSetDisplayWindow(
    uint16_t left,  // start Horizon address
    uint16_t top,   // start Vertical address
    uint16_t right, // end Horizon address
    uint16_t bottom // end Vertical address
)
{
    uint16_t newleft = left;
    uint16_t newtop = top;
    uint16_t newright = right;
    uint16_t newbottom = bottom;
    OSI_LOGI(0, "lcd[st7789v2]:    _stSetDisplayWindow L = %d, top = %d, R = %d, bottom = %d", left, top, right, bottom);

    LCD_CtrlWrite_ST(0x2a);                   // set hori start , end (left, right)
    LCD_DataWrite_ST((newleft >> 8) & 0xff);  // left high 8 b
    LCD_DataWrite_ST(newleft & 0xff);         // left low 8 b
    LCD_DataWrite_ST((newright >> 8) & 0xff); // right high 8 b
    LCD_DataWrite_ST(newright & 0xff);        // right low 8 b

    LCD_CtrlWrite_ST(0x2b);                    // set vert start , end (top, bot)
    LCD_DataWrite_ST((newtop >> 8) & 0xff);    // top high 8 b
    LCD_DataWrite_ST(newtop & 0xff);           // top low 8 b
    LCD_DataWrite_ST((newbottom >> 8) & 0xff); // bot high 8 b
    LCD_DataWrite_ST(newbottom & 0xff);        // bot low 8 b
    LCD_CtrlWrite_ST(0x2c);                    // recover memory write mode
}

/**************************************************************************************/
// Description: initialize all LCD with LCDC MCU MODE and LCDC mcu mode
/**************************************************************************************/
static void _stInit(void)
{
    OSI_LOGI(0, "lcd[st7789v2]:   _stInit ");

    LCD_CtrlWrite_ST(0x11);
    _lcdDelayMs(10);


    LCD_CtrlWrite_ST(0x36);
    LCD_DataWrite_ST(0x00);

    LCD_CtrlWrite_ST(0xb2);
    LCD_DataWrite_ST(0x0c);
    LCD_DataWrite_ST(0x0c);
    LCD_DataWrite_ST(0x00);
    LCD_DataWrite_ST(0x33);
    LCD_DataWrite_ST(0x33);

    LCD_CtrlWrite_ST(0xb7);
    LCD_DataWrite_ST(0x35);

    LCD_CtrlWrite_ST(0xbb);
    LCD_DataWrite_ST(0x37);

    LCD_CtrlWrite_ST(0xc0);
    LCD_DataWrite_ST(0x2c);

    LCD_CtrlWrite_ST(0xc2);
    LCD_DataWrite_ST(0x01);

    LCD_CtrlWrite_ST(0xc3);
    LCD_DataWrite_ST(0x09);   //  07

    LCD_CtrlWrite_ST(0xc4);
    LCD_DataWrite_ST(0x20);

    LCD_CtrlWrite_ST(0xc6);
    LCD_DataWrite_ST(0x03);

    LCD_CtrlWrite_ST(0xd0);
    LCD_DataWrite_ST(0xa4);
    LCD_DataWrite_ST(0xa1);
    
    LCD_CtrlWrite_ST(0xb0);
    LCD_DataWrite_ST(0x00);
    LCD_DataWrite_ST(0xd0);
    
    LCD_CtrlWrite_ST(0xe0);
    LCD_DataWrite_ST(0xd0);
    LCD_DataWrite_ST(0x00);
    LCD_DataWrite_ST(0x05);
    LCD_DataWrite_ST(0x0e);
    LCD_DataWrite_ST(0x15);
    LCD_DataWrite_ST(0x0d);
    LCD_DataWrite_ST(0x37);
    LCD_DataWrite_ST(0x43);
    LCD_DataWrite_ST(0x47);
    LCD_DataWrite_ST(0x09);
    LCD_DataWrite_ST(0x15);
    LCD_DataWrite_ST(0x12);
    LCD_DataWrite_ST(0x16);
    LCD_DataWrite_ST(0x19);

    LCD_CtrlWrite_ST(0xe1);
    LCD_DataWrite_ST(0xd0);
    LCD_DataWrite_ST(0x00);
    LCD_DataWrite_ST(0x05);
    LCD_DataWrite_ST(0x0d);
    LCD_DataWrite_ST(0x0c);
    LCD_DataWrite_ST(0x06);
    LCD_DataWrite_ST(0x2d);
    LCD_DataWrite_ST(0x44);
    LCD_DataWrite_ST(0x40);
    LCD_DataWrite_ST(0x0e);
    LCD_DataWrite_ST(0x1c);
    LCD_DataWrite_ST(0x18);
    LCD_DataWrite_ST(0x16);
    LCD_DataWrite_ST(0x19);

    LCD_CtrlWrite_ST(0x35);
    LCD_DataWrite_ST(0x00);

    LCD_CtrlWrite_ST(0x3A);
    LCD_DataWrite_ST(0x55);


    LCD_CtrlWrite_ST(0x29);

}

static void _stSleepIn(bool is_sleep)
{
    OSI_LOGI(0, "lcd[st7789v2]:  _stSleepIn, is_sleep = %d", is_sleep);

    if (is_sleep)
    {
        LCD_CtrlWrite_ST(0x28); //display off
        _lcdDelayMs(120);
        LCD_CtrlWrite_ST(0x10); // enter sleep mode
    }
    else
    {
        _stInit();
    }
}

static void _stClose(void)
{
    OSI_LOGI(0, "lcd[st7789v2]: in _stClose");

    _stSleepIn(true);
}
static void _stInvalidate(void)
{
    OSI_LOGI(0, "lcd[st7789v2]:  in _stInvalidate");
}

void _stInvalidateRect(
    uint16_t left,  //the left value of the rectangel
    uint16_t top,   //top of the rectangle
    uint16_t right, //right of the rectangle
    uint16_t bottom //bottom of the rectangle
)
{

    OSI_LOGI(0, "lcd[st7789v2]: in _stInvalidateRect");
}

static void _stRotationInvalidateRect(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, lcdAngle_t angle)
{

    OSI_LOGI(0, "lcd[st7789v2]: in stRotationInvalidateRect");
}

static void _stEsdCheck(void)
{

    OSI_LOGI(0, "lcd[st7789v2]: st7789v2 stEsdCheck");
}

static uint32_t _stReadId(void)
{
    uint32_t ret_id = 0;
    HAL_ERR_T r_err;
    uint8_t id[4] = {0};
    OSI_LOGI(0, "lcd[st7789v2]:  st7789v2   _stReadId \n");

    r_err = fibo_lcd_spi_read_data(0x04, id, 4);
    _lcdDelayMs(10);
    OSI_LOGI(0, "lcd[st7789v2]:  _stReadId LCM: 0x%0x, 0x%x, 0x%0x (read return : %d)\n", id[3], id[2], id[1], r_err);

    ret_id = ((id[3] << 16) | (id[2] << 8) | id[1]);

	OSI_LOGI(0, "lcd[st7789v2]: st7789v2 ReadId:0x%x \n",ret_id);
    if (LCD_ID_ST7789v2 == ret_id)
    {
        OSI_LOGI(0, "lcd[st7789v2]:  ID read is correct!");
    }
	else
	{
		OSI_LOGI(0, "lcd[st7789v2]:  ID read is error!");
	}
    
    return ret_id;
}

static const lcdOperations_t st7789v2Operations =
    {
        _stInit,
        _stSleepIn,
        _stEsdCheck,
        _stSetDisplayWindow,
        _stInvalidateRect,
        _stInvalidate,
        _stClose,
        _stRotationInvalidateRect,
        _stSetDirection,
        _stReadId,
};

static const lcdSpec_t g_lcd_st7789v2 =
    {
        LCD_ID_ST7789v2,
        LCD_WIDTH,
        LCD_HEIGHT,
        HAL_GOUDA_SPI_LINE_3,
        LCD_CTRL_SPI,
        (lcdOperations_t *)&st7789v2Operations,
        false,
        0x2a000,
        50000000,
};
static lcdCfg_t g_lcd_config =
    {
	(lcdSpec_t *)&g_lcd_st7789v2,
        LCD_POWER_1V8,
        true,
        0,
        0,
        0,
};
void test_colorbar_buffer(uint16_t *p, int w, int h)
{
    int i,j;
    for(i=0; i<h; i++)
    {
        for(j=0; j<w; j++)
        {
            if(j<w/4)
            {
                p[i*w+j]=0x001f;
            }
            else if (j<w/2)
            {
                p[i*w+j]=0xf000;
            }
            else if (j<w*3/4)
            {
                p[i*w+j]=0x07f0;
            }
            else
            {
                p[i*w+j]=0xf80f;
            }
        }
    }

}


void fill_full_screen(uint16_t *p, int w, int h,uint16_t color)
{
    uint16_t i,j;
    for(i=0;i<h;i++)
    {
        for(j=0;j<w;j++)
        {
            p[i*w+j]=color;
        }
    }
}

void lcd_display_background(uint16_t bgClor)
{
    lcdDisplay_t startPoint;

    startPoint.x=0;
    startPoint.y=0;
    startPoint.width=LCD_WIDTH;
    startPoint.height=LCD_HEIGHT;
	
	fibo_lcd_FillRect(&startPoint, bgClor);
    fibo_taskSleep(100);
}

static uint16_t *g_testBuff_128_160=NULL;
static uint16_t *g_TestBuffer=NULL;	
void lcd_buffer_init(void)
{
    g_testBuff_128_160 = (uint16_t *)fibo_malloc(128 * 160 * 2);
    if (g_testBuff_128_160 == NULL)
    {
        return;
    }
	g_TestBuffer = (uint16_t *)malloc(LCD_WIDTH * LCD_HEIGHT* 2);
    if (g_TestBuffer == NULL)
    {
        return;
    }
}

void lcd_line_test(void)
{
	lcdDisplay_t startPoint;
	startPoint.x=50;
    startPoint.y=100;
    startPoint.width=100;
    startPoint.height=50;
    fibo_lcd_FillRect(&startPoint, GRAY);
    osiThreadSleep(2000);
    
    fibo_lcd_SetPixel(10, 10, RED);
    fibo_lcd_SetPixel(10, 20, GREEN);
    fibo_lcd_DrawLine(1, 170, 200, 310,WHITE);
    osiThreadSleep(1000);
    fibo_lcd_DrawLine(200, 170, 1, 310,WHITE);
    osiThreadSleep(1000);
    fibo_lcd_DrawLine(200, 240, 1, 240,WHITE);
    osiThreadSleep(1000);
    fibo_lcd_DrawLine(100, 310, 100, 170,WHITE);
    
    osiThreadSleep(2000);
}

void lcd_test(void)
{
    lcdDisplay_t startPoint;
    lcdFrameBuffer_t dataWin;
	uint8_t dir[4]={LCD_DIRECT_NORMAL,LCD_DIRECT_ROT_90,LCD_DIRECT_ROT_180,LCD_DIRECT_ROT_270};
	static uint8_t dirloop=0;
    
    dataWin.buffer=(uint16_t *)g_testBuff_128_160;
    dataWin.colorFormat=LCD_RESOLUTION_RGB565;
    dataWin.region_x=0;
    dataWin.region_y=0;
    dataWin.height=128;
    dataWin.width=160;
    dataWin.widthOriginal=160;
    dataWin.keyMaskEnable=false;
    dataWin.maskColor=0;
    dataWin.rotation=0;
    
    startPoint.x=0;
    startPoint.y=0;
    startPoint.height=128;
    startPoint.width=160;
	
    fibo_lcd_FillRect(&startPoint, BLACK);
	dirloop=(dirloop+1)%4;
	fibo_lcd_SetBrushDirection(dir[dirloop]);
    fibo_lcd_FrameTransfer(&dataWin, &startPoint);
	fibo_taskSleep(100);
}
void lcd_over_test(void)
{
	lcdDisplay_t startPoint;
    lcdFrameBuffer_t dataWin;
	lcdFrameBuffer_t dataBufferWin;
	uint16_t color[4]={0x001f,0xf000,0x07f0,0xf80f};
	uint8_t dir[4]={LCD_DIRECT_NORMAL,LCD_DIRECT_ROT_90,LCD_DIRECT_ROT_180,LCD_DIRECT_ROT_270};
	static uint8_t dirloop=0;
	static uint16_t height;
	static uint16_t width;

	if ((dirloop == 0)||(dirloop == 2))
	{
		height=320;
		width=240;
	}
	else
	{
		height=240;
		width=320;
	}
	
    fill_full_screen(g_TestBuffer,width,height,color[dirloop]);
    dataBufferWin.buffer=(uint16_t *)g_TestBuffer;
    dataBufferWin.colorFormat=LCD_RESOLUTION_RGB565;
    dataBufferWin.region_x=0;
    dataBufferWin.region_y=0;
    dataBufferWin.height=height;
    dataBufferWin.width=width;
    dataBufferWin.keyMaskEnable=false;
    dataBufferWin.maskColor=0;
    dataBufferWin.rotation=0;
    fibo_lcd_SetOverLay(&dataBufferWin);


    dataWin.buffer=(uint16_t *)g_testBuff_128_160;
    dataWin.colorFormat=LCD_RESOLUTION_RGB565;
    dataWin.region_x=0;
    dataWin.region_y=0;
    dataWin.height=128;
    dataWin.width=160;
    dataWin.keyMaskEnable=false;
    dataWin.maskColor=0;
    dataWin.rotation=0;
    
    startPoint.x=0;
    startPoint.y=0;
    startPoint.height=height;
    startPoint.width=width;

	
	fibo_lcd_SetBrushDirection(dir[dirloop]);
    fibo_lcd_FrameTransfer(&dataWin, &startPoint);
	dirloop=(dirloop+1)%4;
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    
    fibo_taskSleep(4000);
	fibo_gpio_mode_set(35, 0);
	fibo_gpio_set(35,1);  //open backlight
	fibo_taskSleep(1000);
	fibo_lcd_register(&g_lcd_config, NULL);
    fibo_lcd_init();
	lcd_display_background(BLACK);
	lcd_line_test();
	lcd_display_background(BLACK);

	lcd_buffer_init();
	test_colorbar_buffer(g_testBuff_128_160,160,128);
    while(1)
    {
		lcd_over_test();
		fibo_taskSleep(1000);
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
