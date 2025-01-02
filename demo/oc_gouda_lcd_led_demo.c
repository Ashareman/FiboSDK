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

//color code
#define WHITE            0xFFFF
#define BLACK            0x0000   
#define BLUE             0x001F  
#define BRED             0XF81F
#define GRED             0XFFE0
#define GBLUE            0X07FF
#define RED              0xF800
#define MAGENTA          0xF81F
#define GREEN            0x07E0
#define CYAN             0x7FFF
#define YELLOW           0xFFE0
#define BROWN            0XBC40 //棕色
#define BRRED            0XFC07 //棕红色
#define GRAY             0X8430 //灰色

extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

#define     DLED_LINE_LENGTH     (7)
#define     DLED_SPI_CLK         (2000000)
#define     LCD_SPI_CLK          (50000000)

static uint8_t _byte_reverse(uint8_t byte);

const uint8_t segbuf[]			={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x40}; //0~9共阴，不带小数点的段码，最后一位为单独的"-"号
const uint8_t segbufpoint[]	    ={0xbf,0x86,0xdb,0xcf,0xe6,0xed,0xfd,0x87,0xff,0xef,0x80}; //0~9共阴，带小数点的段码，最后一位为单独的一个小数点

#define LED_CtrlWrite(Cmd)                            \
    {                                                 \
		uint8_t ucdat;                                \
		ucdat = _byte_reverse(Cmd);                   \
        while (fibo_lcd_spi_write_cmd(ucdat) != 0)    \
            ;                                         \
    }

struct digitalled_app
{
	uint16_t uiLine; // 对应显示的行数
	uint8_t ucDisplayMode; // 对应的显示模式（左对齐，右对齐）
	char acData[64]; // 对应显示的数据
};
enum display_mode
{
    DLED_DISPLAY_ALIGN_LEFT = 0x31,
    DLED_DISPLAY_ALIGN_RIGHT,
};

static uint32_t lcdLock=0;

static void LcdGetSemaphore(void)
{
    fibo_sem_wait(lcdLock);
}

static void LcdPutSemaphore(void)
{
    fibo_sem_signal(lcdLock);
}

static void _lcdDelayMs(int ms_delay)
{
    osiDelayUS(ms_delay * 1000);
}
static uint8_t _byte_reverse(uint8_t byte)
{
    uint8_t ucTemp = 0x00;
    uint8_t ucData = 0x00;
    uint8_t i;

	for(i=0;i<8;i++)   
    {        
        ucTemp= (byte & (0x1 << i));                
		if (ucTemp != 0)                
		{
			ucData |= (1 << (7-i));    
		}
	}
	OSI_LOGI(0, "lcd _byte_reverse:byte=0x%02x data=0x%02x", byte, ucData);
	return ucData;
}
static  void _LED_CSinit(void)
{
	fibo_gpio_mode_set(56,1); //set pin56 gpio
	fibo_gpio_cfg(56,1);  //set pin56 output
	fibo_gpio_set(56,1);  //set pin56 output high
}

static  void _LED_vStart(void)
{
	fibo_gpio_set(56,0);  //set pin56 output low
}

static  void _LED_vStop(void)
{
    osiDelayUS(4);
    fibo_gpio_set(56,1);  //set pin56 output high
    osiDelayUS(2);
}

static  void _LCD_cs_enable(void)
{
	fibo_gpio_mode_set(52,0); //set pin52 for Lcd_cs
}
static  void _LCD_cs_disable(void)
{
	fibo_gpio_mode_set(52,1); //set pin52 for gpio
	fibo_gpio_cfg(52,1);  //set pin52 output
	fibo_gpio_set(52,1);  //set pin52 output high
}

void _istBL_set(bool flag)
{
	if(flag)
	{
		_LED_vStart();
		LED_CtrlWrite(0x41);
		LED_CtrlWrite(0xF0);//0xc0
		_LED_vStop();
	}
	else
	{
		_LED_vStart();
		LED_CtrlWrite(0x41);
		LED_CtrlWrite(0x00);
		_LED_vStop();
	}
}

//  将传统的数码管显示码转换为IST3612的显示格式
uint8_t _ist3612_DispBufConvert(uint8_t *pucData, uint32_t ulLen, uint8_t *pucConvertDataOut, uint8_t ucDispMode)
{
    int32_t i = 0, j = 0;
    int32_t iLenTmp;
    uint8_t aucDispBufTmp[DLED_LINE_LENGTH];
    uint8_t aucConvertBuf[4][2]; 

	if ((pucData==NULL) || (pucConvertDataOut==NULL))
    {
        return -1;
    }

    memset(aucConvertBuf, 0, sizeof(aucConvertBuf));
    memset(aucDispBufTmp, 0, sizeof(aucDispBufTmp));

    iLenTmp = (ulLen - DLED_LINE_LENGTH);
    if(ucDispMode == DLED_DISPLAY_ALIGN_RIGHT)//  右对齐
    {
        if(iLenTmp > 0)//  能够整行显示，取高EM_dled_LINE_LENGTH 的长度到显示缓冲区中
        {
            memcpy(aucDispBufTmp, pucData + iLenTmp, DLED_LINE_LENGTH);
        }
        else//  无法整行显示，则取全部内容到buf[iLenTmp, EM_dled_LINE_LENGTH-1]显示缓冲区中，剩余的缓冲区内容为空
        {
            iLenTmp = (DLED_LINE_LENGTH - ulLen);
            memcpy(aucDispBufTmp + iLenTmp , pucData, ulLen);
        }
    }
    else//  左对齐
    {
        if(iLenTmp > 0)//  能够整行显示，取高EM_dled_LINE_LENGTH 的长度到显示缓冲区中
        {
            memcpy(aucDispBufTmp, pucData, DLED_LINE_LENGTH);
        }
        else//  无法整行显示，则取全部内容到buf[0, iLenTmp]显示缓冲区中，剩余的缓冲区内容为空
        {
            iLenTmp = ulLen;
            memcpy(aucDispBufTmp , pucData, ulLen);
        }
    }
   // EI_dumpHex("pucData", pucData, uiLen);
  //  EI_dumpHex("aucDispBufTmp",aucDispBufTmp, EM_dled_LINE_LENGTH);
    for(i = DLED_LINE_LENGTH - 1, j = 0; i >= 0; i--, j++)
    {
     //   PRINTF("i:%d j:%d\r\n", i, j);
      /*  PRINTF("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\r\n",   ((aucDispBufTmp[i] & (0x01 << 0)) >> 0) << (1 + 2 * (j%4)),
                                                                ((aucDispBufTmp[i] & (0x01 << 1)) >> 1) << (0 + 2 * (j%4)),
                                                                ((aucDispBufTmp[i] & (0x01 << 2)) >> 2) << (0 + 2 * (j%4)),
                                                                ((aucDispBufTmp[i] & (0x01 << 3)) >> 3) << (1 + 2 * (j%4)),
                                                                ((aucDispBufTmp[i] & (0x01 << 4)) >> 4) << (1 + 2 * (j%4)),
                                                                ((aucDispBufTmp[i] & (0x01 << 5)) >> 5) << (1 + 2 * (j%4)),
                                                                ((aucDispBufTmp[i] & (0x01 << 6)) >> 6) << (0 + 2 * (j%4)),
                                                                ((aucDispBufTmp[i] & (0x01 << 7)) >> 7) << (0 + 2 * (j%4)));
        */                                                                
        aucConvertBuf[0][j/4] |= ((aucDispBufTmp[i] & (0x01 << 0)) >> 0) << (1 + 2 * (j%4));//  取A段
        aucConvertBuf[0][j/4] |= ((aucDispBufTmp[i] & (0x01 << 1)) >> 1) << (0 + 2 * (j%4));//  取B段
        aucConvertBuf[2][j/4] |= ((aucDispBufTmp[i] & (0x01 << 2)) >> 2) << (0 + 2 * (j%4));//  取C段
        aucConvertBuf[3][j/4] |= ((aucDispBufTmp[i] & (0x01 << 3)) >> 3) << (1 + 2 * (j%4));//  取D段
        aucConvertBuf[2][j/4] |= ((aucDispBufTmp[i] & (0x01 << 4)) >> 4) << (1 + 2 * (j%4));//  取E段
        aucConvertBuf[1][j/4] |= ((aucDispBufTmp[i] & (0x01 << 5)) >> 5) << (1 + 2 * (j%4));//  取F段
        aucConvertBuf[1][j/4] |= ((aucDispBufTmp[i] & (0x01 << 6)) >> 6) << (0 + 2 * (j%4));//  取G段
        aucConvertBuf[3][j/4] |= ((aucDispBufTmp[i] & (0x01 << 7)) >> 7) << (0 + 2 * (j%4));//  取S段
    }

   
    memcpy(pucConvertDataOut, aucConvertBuf, sizeof(aucConvertBuf));   
    //EI_dumpHex("EG_dled_aucDispBuf", (uchar*)aucConvertBuf, sizeof(aucConvertBuf));
    //EI_dumpHex("pucConvertDataOut",pucConvertDataOut, sizeof(aucConvertBuf));
    return 0;

}


int32_t _istSendDisplayData(uint8_t *pucDispData, uint32_t ulLen, void * pvParam)
{
    uint32_t i;

    
    if(pucDispData == NULL)
    {
        return -1;
    }

    _LED_vStart();
    LED_CtrlWrite(0x40); // 设置为地址+1模式
    _LED_vStop();

	_LED_vStart();
    LED_CtrlWrite(0xC0); // 设置起始地址为第一个数码管地址
    for(i = 0; i < ulLen; i++)
    {
        LED_CtrlWrite(pucDispData[i]);
    }
    _LED_vStop();

	_LED_vStart();
	LED_CtrlWrite(0x00);
    LED_CtrlWrite(0x88);
    _LED_vStop();
	
    return 0;
}


int32_t _istLEDDisplay(struct digitalled_app dig_app)
{
    int32_t i,len,DispNum;
	uint8_t aucLedDisplayBuf[64] = {0};
	int32_t lRet =0;
    uint8_t aucLedDisplayConvertBuf[DLED_LINE_LENGTH+1];

    memset(aucLedDisplayConvertBuf, 0, sizeof(aucLedDisplayConvertBuf));

	// 如果ucDisplayMode不是ALIGN_RIGHT和ALIGN_LEFT，则返回参数错误
	if((dig_app.ucDisplayMode != DLED_DISPLAY_ALIGN_RIGHT) && (dig_app.ucDisplayMode != DLED_DISPLAY_ALIGN_LEFT))
		return -1;

	// 行数错误，视板级情况，返回设备不存在
	//if(dig_app.uiLine > pdata->led_line_number || dig_app.uiLine < 1)
	//	return -2;
	
	len = strlen(dig_app.acData);
	OSI_PRINTFI("lcd app_data:len=%d data=%s", len, dig_app.acData);

	memset(aucLedDisplayBuf, 0x00, sizeof(aucLedDisplayBuf));
	//将字符串中数字转换为数码管显示码，存放在aucLedDisplayBuf中
	
		for(i=0, DispNum=0;i<len;i++)
		{
			//字符串中有小数点，作特殊显示
			if(dig_app.acData[i] == '.')
			{
				if(i == 0) // 第一位是小数点，则只显示小数点
				{
					aucLedDisplayBuf[DispNum] = segbufpoint[10];
				}
				else if(dig_app.acData[i-1] == '.') //连续出现两个小数点
				{
					aucLedDisplayBuf[DispNum] =  aucLedDisplayBuf[DispNum] | segbufpoint[10];
				}
				else
				{
					aucLedDisplayBuf[DispNum-1] = aucLedDisplayBuf[DispNum-1] | segbufpoint[10];
		            continue;
				}
			}
			// "-"符号，正常显示，段码在seg_buf[10]中存放
			else if(dig_app.acData[i] == '-')
			{
				aucLedDisplayBuf[DispNum] = segbuf[10];
			}
			// 0~9数据正常显示
			else if(dig_app.acData[i] >= '0' && dig_app.acData[i] <= '9')
			{
				aucLedDisplayBuf[DispNum] = segbuf[dig_app.acData[i] - 0x30];	
			}
			// 如果数据中含有非0~9，且不是小数点的数据，则该位作为空格显示
			else
			{
				lRet = -3;
			}
			DispNum++;
		}

		//if(DispNum > pdata->line_device[dig_app.uiLine-1].led_line_length)
		//	lRet = -4;

		for(i=0;i<DispNum;i++)
		{
			OSI_PRINTFI("lcd aucLedDisplayBuf: data=0x%02x i=%d", aucLedDisplayBuf[i], i);
		}
		_ist3612_DispBufConvert(aucLedDisplayBuf, DispNum, aucLedDisplayConvertBuf, dig_app.ucDisplayMode);
		lRet= _istSendDisplayData(aucLedDisplayConvertBuf, 8, NULL);
		_istBL_set(true);
	return lRet;
}

// 熄灭所有数码管，并清零所有寄存器，相当于对芯片进行复位，用于重启和刚加载驱动时的控制
 void _istDLED_init(void)
{
	int i;
	struct digitalled_app dig_app;

	LcdGetSemaphore();
	
    _LCD_cs_disable();
	fibo_lcd_spi_set_freq(DLED_SPI_CLK);
	_LED_CSinit();
	_lcdDelayMs(200);

	_LED_vStart();
    LED_CtrlWrite(0x74);
    LED_CtrlWrite(0x74);
    LED_CtrlWrite(0x74);
    LED_CtrlWrite(0x74);  //IST Mode


    LED_CtrlWrite(0x2F);  //Power on   2F
    LED_CtrlWrite(0x81);  //V0 set
    LED_CtrlWrite(0x11);  //11=6.0  

    LED_CtrlWrite(0xF2);  //FR SET
    LED_CtrlWrite(0x31);  // 180hz 31
    LED_CtrlWrite(0x10);  //   10 

    LED_CtrlWrite(0xf8);
    LED_CtrlWrite(0x00);  //Bias=1/3

    LED_CtrlWrite(0x22);  //RR=4.0 0x22 

    LED_CtrlWrite(0xB2);  //BT=4X

	LED_CtrlWrite(0x60);  //sleep out

    LED_CtrlWrite(0x30);  //Exit
    
	_LED_vStop();
		
	_LED_vStart();
	LED_CtrlWrite(0x40);
	_LED_vStop();

	_LED_vStart();
	LED_CtrlWrite(0xC0);
	// 所有数据寄存器清零，相当于所有数码管段显示消隐
	for(i=0;i< 8;i++)
	{	
		LED_CtrlWrite(0x00);
	}
	_LED_vStop();

	_LED_vStart();
	LED_CtrlWrite(0x00);
	LED_CtrlWrite(0x88);
	_LED_vStop(); 

	_istBL_set(true);

    memset(dig_app.acData, 0, 64);
	strcpy(dig_app.acData, "0.0.0.0.0.0.0");
	dig_app.ucDisplayMode=DLED_DISPLAY_ALIGN_RIGHT;
	dig_app.uiLine=1;

	_istLEDDisplay(dig_app);
	_lcdDelayMs(100);
	_LCD_cs_enable();
	fibo_lcd_spi_set_freq(LCD_SPI_CLK);
	
	LcdPutSemaphore();
	return;
}

void dled_display(char *numb)
{
	struct digitalled_app dig_app;

	if (numb==NULL)
		return;
	if (strlen(numb)>63)
		return;
	LcdGetSemaphore();
	
    _LCD_cs_disable();
	fibo_lcd_spi_set_freq(DLED_SPI_CLK);
	
	memset(dig_app.acData, 0, 64);
	strcpy(dig_app.acData, numb);
	dig_app.ucDisplayMode=DLED_DISPLAY_ALIGN_RIGHT;
	dig_app.uiLine=1;

	_istLEDDisplay(dig_app);
    _lcdDelayMs(100);
	_LCD_cs_enable();
	fibo_lcd_spi_set_freq(LCD_SPI_CLK);
	
	LcdPutSemaphore();
}

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


void fill_word_buff(uint16_t *p, int w, int h)
{
    uint16_t i,j;
    for(i=0;i<h;i++)
    {
        for(j=0;j<w;j++)
        {
            p[i*w+j]=0xf800;
        }
    }

    for(i=10;i<120;i++)
    {
        for(j=80;j<90;j++)
        {
            p[i*w+j]=0x001f;
        }
    }
    for(i=10;i<120;i++)
    {
        for(j=180;j<190;j++)
        {
            p[i*w+j]=0x001f;
        }
    }
    for(i=60;i<70;i++)
    {
        for(j=90;j<180;j++)
        {
            p[i*w+j]=0x001f;
        }
    }
}

void lcd_display_background(uint16_t bgClor)
{
    lcdDisplay_t startPoint;

    startPoint.x=0;
    startPoint.y=0;
    startPoint.width=240;
    startPoint.height=320;
	
	LcdGetSemaphore();
	
	fibo_lcd_FillRect(&startPoint, bgClor);
    fibo_taskSleep(100);
	
	LcdPutSemaphore();
}

static uint16_t *g_testBuff_128_160=NULL;
	
void lcd_buffer_init(void)
{
    g_testBuff_128_160 = (uint16_t *)fibo_malloc(128 * 160 * 2);
    if (g_testBuff_128_160 == NULL)
    {
        return;
    }
}

void lcd_test(void)
{
    lcdDisplay_t startPoint;
    lcdFrameBuffer_t dataWin;
	uint8_t dir[4]={LCD_DIRECT_NORMAL,LCD_DIRECT_ROT_90,LCD_DIRECT_ROT_180,LCD_DIRECT_ROT_270};
	static uint8_t dirloop=0;
    
    test_colorbar_buffer(g_testBuff_128_160,160,128);
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

	LcdGetSemaphore(); 
	
    fibo_lcd_FillRect(&startPoint, BLACK);
	dirloop=(dirloop+1)%4;
	fibo_lcd_SetBrushDirection(dir[dirloop]);
    fibo_lcd_FrameTransfer(&dataWin, &startPoint);
	fibo_taskSleep(100);
	
	LcdPutSemaphore();
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
	uint32_t i=0;
	char numlist[64];

	if(lcdLock == 0)
	{
		lcdLock=fibo_sem_new(1);
	}
    fibo_lcd_rstPin_setLevel(71,0); //for liandi
	fibo_hal_pmu_setlevel(0,1);     //for liandi
    fibo_lcd_init();
	lcd_display_background(BLACK);
	fibo_taskSleep(1000);
	//fibo_lcd_spi_config(0, 2000000); //if not lcd, use this api init
	_istDLED_init();
	fibo_taskSleep(2000);
	lcd_buffer_init();

	while(1)
	{
		i++;
		memset(numlist,0,64);
		sprintf(numlist,"%ld.0",i);
		dled_display(numlist);
		fibo_taskSleep(100);
		lcd_test();
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
