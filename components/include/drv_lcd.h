/* This software is supplied "AS IS" without any warranties.
* RDA assumes no responsibility or liability for the use of the software,
* conveys no license or title under any patent, copyright, or mask work
* right to the product. RDA reserves the right to make changes in the
* software without notification.  RDA also make no representation or
* warranty that such application will be suitable for the specified use
* without further testing or modification.
*/

#ifndef _DRV_LCD_H_
#define _DRV_LCD_H_

#include "osi_compiler.h"

OSI_EXTERN_C_BEGIN

#include "hal_gouda.h"
#include "fibocom.h"
#ifdef CONFIG_FIBOCOM_BASE
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

//Err Code
#define   DRV_LCD_SUCCESS                 0
#define   DRV_LCD_NOT_INIT               (-1)
#define   DRV_LCD_HAS_INITED             (-2)
#define   DRV_LCD_SEM_CREATE_FAILED      (-3)
#define   DRV_LCD_GET_SPEC_FAILED        (-4)
#define   DRV_LCD_GET_OPERATION_FAILED   (-5)
#define   DRV_LCD_COLOR_FORMAT_ERROR     (-6)
#define   DRV_LCD_FRAME_BUFFER_ERROR     (-7)
#define   DRV_LCD_CONFIG_BLOCK_FAILED    (-8)
#define   DRV_LCD_SET_WINDOW_FAILED      (-9)
#define   DRV_LCD_INVALID_PARAMETER      (-10)
#define   DRV_LCD_SET_BRUSH_FAILED       (-11)
#endif

#if defined(CONFIG_DOT_MATRIX_LCD_SUPPORT)
#define DOT_LCD_ID_UNSUPPORT            (0X55)  // un-support read ID case
#if defined(CONFIG_AITI_CUS_LCD_SUPPORT)
#define AITI_DOT_LCD_ID_ST7571          (0X01)
#define AITI_DOT_LCD_ID_UC1617S         (0X00)
#endif
#endif


typedef enum
{
    LCD_ANGLE_0,
    LCD_ANGLE_90,
    LCD_ANGLE_180,
    LCD_ANGLE_270
} lcdAngle_t;

typedef enum
{
    LCD_DIRECT_NORMAL = 0x00,
    LCD_DIRECT_ROT_90, // Rotation 90
#ifdef CONFIG_FIBOCOM_BASE
    LCD_DIRECT_ROT_180, // Rotation 180
    LCD_DIRECT_NORMAL2,
    LCD_DIRECT_NORMAL2_ROT_90,
    LCD_DIRECT_NORMAL2_ROT_180,
    LCD_DIRECT_ROT_270,  // Rotation 270
    LCD_DIRECT_NORMAL2_ROT_270,
#endif

#ifdef CONFIG_SUPPORT_MIPI_LCD
    /** normal direction */
    DRV_LCD_DIR_NORMAL,
    /** Y is inverted */
    DRV_LCD_DIR_YINV,
    /** X is inverted */
    DRV_LCD_DIR_XINV,
    /** both Y and X are inverted */
    DRV_LCD_DIR_YINV_XINV,
#endif

/* Direction of dot matrix LCD's flushing data to LCM */
#ifdef CONFIG_SUPPORT_GENSPI_LCD
    /* normal direction: SEG0 -> SEG131  COM0 -> COM63 */
    DMLCD_DIRECT_NORMAL,
    /* Y inverted: COM63 -> COM0 */
    DMLCD_DIRECT_MY,
    /* X inverted: SEG131 -> SEG0 */
    DMLCD_DIRECT_MX,
    /* Y and X inverted: SEG131 -> SEG0 COM63 -> COM0 */
    DMLCD_DIRECT_MY_MX,
#endif
} lcdDirect_t;

// LCD Contrller type
typedef enum
{
    LCD_CTRL_SPI = 0, //
    LCD_CTRL_CSI
} LCD_CTRL_MODULE_E;
#ifdef CONFIG_FIBOCOM_BASE
// LCD power domain
typedef enum
{
    LCD_POWER_1V8 = 0, //
    LCD_POWER_3V3
} LCD_POWER_DOMAIN_E;
#endif
//Block Configuration
typedef struct
{
    uint16_t start_x;
    uint16_t start_y;
    uint16_t end_x;
    uint16_t end_y;
    uint16_t colorkey;
    uint16_t width;     //real data width in SDRAM
    uint8_t resolution; //0:RGB888; 1:RGB666; 2:RGB565(RGB555 if OSD)
    uint8_t type;       //0:OSD; 1:image
    uint8_t alpha;
    uint8_t colorkey_en;
    uint32_t rotation;
} lcdBlockCfg_t;

typedef struct
{
    void (*init)(void);
    void (*enterSleep)(bool is_sleep);
    void (*checkEsd)(void);
    void (*setDisplayWindow)(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
    void (*invlaidateRect)(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
    void (*invalidate)(void);
    void (*close)(void);
    void (*rotationInvalidateRect)(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom, lcdAngle_t angle);
    void (*setBrushDirection)(lcdDirect_t direct_type);
    uint32_t (*readId)(void);
	
#if defined(CONFIG_DOT_MATRIX_LCD_SUPPORT)||defined(CONFIG_LCD_ST7571_SUPPORT) || defined(CONFIG_LCD_UC1617S_SUPPORT)
    // only for dot matrix LCD
    int32_t (*dotSetPixel)(uint16_t x, uint16_t y, uint8_t val);
    void (*dotSendBuff)(uint8_t row, uint8_t column, void *buff, uint32_t size);
    void (*dotSetDisplayParam)(uint8_t contrastRatio_coarse, uint8_t contrastRatio_fine, uint8_t bias);

    void (*dotSetAddrOnly)(uint8_t column, uint8_t row);
    void (*dotSendBuffOnly)(void *buff, uint32_t size);
    void (*dotSetFlushDirection)(uint8_t direction);

    // using for uc1617s, for now
    void (*dotSetLcmRate)(uint8_t rate);
#endif
} lcdOperations_t;

typedef struct
{
    uint32_t dev_id;
    uint32_t width;
    uint32_t height;
    HAL_GOUDA_SPI_LINE_T bus_mode;
    LCD_CTRL_MODULE_E controller;
    lcdOperations_t *operation;
    bool is_use_fmark;
    uint32_t fmark_delay;
    uint32_t freq;
#if defined(CONFIG_DOT_MATRIX_LCD_SUPPORT)
    uint8_t *lcdbuff;
    uint32_t lcdbuffsize;
#endif
} lcdSpec_t;

#ifdef CONFIG_FIBOCOM_BASE
typedef struct
{
    lcdSpec_t *lcd_spec;
	LCD_POWER_DOMAIN_E pwr_domain;
	bool is_vertical;
	uint32_t reserved;
	uint32_t reserved2;
	uint32_t reserved3;
} lcdCfg_t;
#endif
//=============================================================================
// lcdDisplay_t
//-----------------------------------------------------------------------------
/// Region of interest in a LCD screen or a buffer.
//=============================================================================
typedef struct
{
    /// x coordinate of the top left corner of the lcd.
    uint16_t x;
    /// y coordinate of the top left corner of the lcd.
    uint16_t y;
    /// width of the lcd dislay area.
    uint16_t width;
    /// height of the lcd display area.
    uint16_t height;

} lcdDisplay_t;

typedef struct
{
    /// Width of the region.
    uint16_t width;
    /// Height of the region.
    uint16_t height;
} lcdRoi_t;

//resolution of lcd data

#define LCD_RESOLUTION_RGB565 2
#define LCD_RESOLUTION_YUV422_UYVY 5
#define LCD_RESOLUTION_YUV420 6
#define LCD_RESOLUTION_YUV422_YUYV 9
#ifdef CONFIG_FIBOCOM_BASE
#define LCD_RESOLUTION_YUV420_3PLANE 8
#endif
//=============================================================================
// lcdFrameBuffer_t
//-----------------------------------------------------------------------------
/// Frame buffer type: this type describes a frame buffer through its dimensions
/// and a pointer to the data of the buffer. It is mandatory for the buffer to
/// be able to hold the data describing all the pixels at the color format
/// choosen.
///(ie sizeof(buffer) = width*height*NbOfBytesPerPixelAtTheFormat(format))
// colorFormat : support LCD_RESOLUTION_RGB565 ,LCD_RESOLUTION_YUV422 LCD_RESOLUTION_YUV420
//widthOrignal  : the width of the original image buffer,it will just be used to caculate the stribe
//width : the width will convert to. Normal is will equal with widthOrignal,unless zoom,rotation.
//height : the height will convert to.
//=============================================================================
typedef struct
{
    uint16_t *buffer;
    ///  x coordinate of the layer. If only use one layer,normal set 0.
    uint16_t region_x;
    ///  y coordinate of the layer. If only use one layer,normal set 0.
    uint16_t region_y;
    ///the width of after convert. if don't need rotation and zoom ,set to  roginal width.
    uint16_t width;
    ///the height of after convert. if don't need rotation and zoom ,set to  roginal height.
    uint16_t height;
    ///image buffer width.
    uint16_t widthOriginal;
    uint16_t colorFormat;
    ///0 no rotation. 1 rotate 90 degree.
    uint16_t rotation;
    /// enable the color mask function.
    bool keyMaskEnable;
    /// mask color value
    uint16_t maskColor;
} lcdFrameBuffer_t;

/**
 * @brief Init the LCD moudle.
 * @return
 *      false  failure
 *      true  success.
 */
#ifdef CONFIG_FIBOCOM_BASE
int drvLcdInit(void);
#else
bool drvLcdInit(void);
#endif

/**
 * @brief The function is used to close lcd when power off.
 */
void drvLcdClose(void);

/**
 * @brief This function provides the basic bit-block transfer capabilities.
 * copies the data (such as characters/bmp) on the LCD directly
 * as a (rectangular) block. The data is drawn in the active window.
 * The buffer has to be properly aligned
 * @param dataBufferWin  set the buffer address , height, width .
 * @param lcdRec   set the position of lcd region.
 * @return
 *      - true on success else false
 */
#ifdef CONFIG_FIBOCOM_BASE
int drvLcdBlockTransfer(const lcdFrameBuffer_t *dataBufferWin, const lcdDisplay_t *lcdRec);
#else
bool drvLcdBlockTransfer(const lcdFrameBuffer_t *dataBufferWin, const lcdDisplay_t *lcdRec);
#endif

/**
 * @brief This function to config the belower layer/vedio layer.
 * @param dataBufferWin  set the buffer address , height, width,etc .
 * @return
 *      - true on success else false
 */
#ifdef CONFIG_FIBOCOM_BASE
int drvLcdBlockSetOverLay(const lcdFrameBuffer_t *dataBufferWin);
#else
bool drvLcdBlockSetOverLay(const lcdFrameBuffer_t *dataBufferWin);
#endif

/**
 * @brief This function get the basic information of lcd.
 * @param lcd_info_ptr  point to store lcd information
 */
void drvLcdGetLcdInfo(lcdSpec_t *lcd_info_ptr);

/**
 * @brief Set the lcd display window.
 * @param  left of the display window
 * @param  top of the display window
 * @param  right of the display window
 * @param  bottom of the display window
 * @return
 *      - true on success else false
 */
bool drvLcdSetDisplayWindow(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);

/**
 * @brief Enable/Disable the lcd module low power mode.
 * @param  left of the display window
 * @param  top of the display window
 * @param  right of the display window
 * @param  bottom of the display window
 * @return
 *      - true on success else false
 */
void drvLcdEnterSleep(bool is_sleep);
#ifdef CONFIG_FIBOCOM_BASE
bool drvLcdSetImageLayerSize(uint32_t width, uint32_t height, uint32_t imageBufWidth);
int drvlcdFillRect16(const lcdDisplay_t * startPoint, uint16_t bgColor);
int drvlcdSetPixel16(uint16_t x, uint16_t y, uint16_t pixelData);
bool drvLcdSetBrushDirection(lcdDirect_t direct_type);
int TestlcdDrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void drvLcdGoudaInit(HAL_GOUDA_SPI_LINE_T spiLineType, uint32_t spiclk);
void drvLcdGoudaSetSpiFreq(uint32_t spiclk);
int32_t drvLcdGoudaWriteCmd(uint16_t cmd);
void drvLcdGoudaSerialMode(HAL_GOUDA_SPI_LINE_T spiLineType);
int32_t drvLcdGoudaReadData(uint16_t addr, uint8_t * pData, uint32_t len);
int32_t drvLcdGoudaWriteData(uint16_t data);
int drvLcdRegister(lcdCfg_t * lcd, void * data);
#endif

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/

OSI_EXTERN_C_END

#endif //_DAL_LCD_H
