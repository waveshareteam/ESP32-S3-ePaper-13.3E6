#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "fonts.h"
#include "imgdecode_app.h"



/**********************************
EPD 13inch3 E6
**********************************/
#define PSR             0x00
#define PWR             0x01
#define POF             0x02
#define PON             0x04
#define BTST_N          0x05
#define BTST_P          0x06
#define DTM             0x10
#define DRF             0x12
#define CDI             0x50
#define TCON            0x60
#define TRES            0x61
#define AN_TM           0x74
#define AGID            0x86
#define BUCK_BOOST_VDDN 0xB0
#define TFT_VCOM_POWER  0xB1
#define EN_BUF          0xB6
#define BOOST_VDDP_EN   0xB7
#define CCSET           0xE0
#define PWS             0xE3
#define CMD66           0xF0


enum ColorSelection {
    ColorBlack = 0,    
    ColorWhite,
    ColorYellow,
    ColorRed,
    ColorBlue = 5,
    ColorGreen
};

/*Bitmap file header   14bit*/
typedef struct BMP_FILE_HEADER {
    uint16_t bType;                         //File identifier
    uint32_t bSize;                         //The size of the file
    uint16_t bReserved1;                    //Reserved value, must be set to 0
    uint16_t bReserved2;                    //Reserved value, must be set to 0
    uint32_t bOffset;                       //The offset from the beginning of the file header to the beginning of the image data bit
} __attribute__((packed)) BMPFILEHEADER;  // 14bit


/*Bitmap information header  40bit*/
typedef struct BMP_INFO {
    uint32_t biInfoSize;       //The size of the header
    uint32_t biWidth;          //The width of the image
    uint32_t biHeight;         //The height of the image
    uint16_t biPlanes;         //The number of planes in the image
    uint16_t biBitCount;       //The number of bits per pixel
    uint32_t biCompression;    //Compression type
    uint32_t bimpImageSize;    //The size of the image, in bytes
    uint32_t biXPelsPerMeter;  //Horizontal resolution
    uint32_t biYPelsPerMeter;  //Vertical resolution
    uint32_t biClrUsed;        //The number of colors used
    uint32_t biClrImportant;   //The number of important colors
} __attribute__((packed)) BMPINFOHEADER;

class ePaperPort {
  private:
    spi_device_handle_t spi;
    uint32_t            i2c_data_pdMS_TICKS = 0;
    uint32_t            i2c_done_pdMS_TICKS = 0;
    const char         *TAG                 = "Display";
    const char         *img_to_bmpName      = "/sdcard/img/sys_decode.bmp";
    ImgDecodeDither &dither_;
    int                 mosi_;
    int                 scl_;
    int                 dc_;
    int                 csm_;
    int                 css_;
    int                 rst_;
    int                 busy_;
    uint16_t            width_;
    uint16_t            height_;
    uint32_t            e_paper_size_;
    uint32_t            e_paper_bmp_size_;
    uint16_t            scale_MaxWidth_;
    uint16_t            scale_MaxHeight_;
    uint8_t            *DispBuffer = NULL;
    uint8_t            *RotationBuffer = NULL;
    uint8_t            *BmpSrcBuffer = NULL;
    uint32_t            DisplayLen;
    uint16_t            src_width;
    uint16_t            src_height;
    uint8_t Rotation = 0;                          //0:0 1:90 2:180 3:270
    uint8_t mirrx = 0;                             
    uint8_t mirry = 0;
    bool isEPDInit = false;

    void    Set_ResetIOLevel(uint8_t level);
    void    Set_CSMIOLevel(uint8_t level);
    void    Set_CSSIOLevel(uint8_t level);
    void    Set_CSALLIOLevel(uint8_t level);
    void    Set_DCIOLevel(uint8_t level);
    uint8_t Get_BusyIOLevel();
    void    EPD_Reset(void);
    void    EPD_LoopBusy(void);
    void    SPI_Write(uint8_t data);
    void    EPD_SendCommand(uint8_t Reg);
    void    EPD_SendData(uint8_t Data);
    void    EPD_Sendbuffera(const uint8_t *Data, uint32_t len);
    void    EPD_SPI_Send(uint8_t Cmd, const uint8_t *buf, uint32_t Len);
    void    EPD_TurnOnDisplay(void);
    uint8_t EPD_ColorToePaperColor(uint8_t b,uint8_t g,uint8_t r);
    uint8_t* EPD_ParseBMPImage(const char *path);
    uint8_t EPD_GetPixel4(const uint8_t* buf, int width, int x, int y);
    void    EPD_SetPixel4(uint8_t* buf, int width, int x, int y, uint8_t px);
    void EPD_PixelRotate();
    void EPD_DrawChar(uint16_t Xpoint, uint16_t Ypoint, const char Acsii_Char,sFONT* Font, uint16_t Color_Foreground, uint16_t Color_Background);

  public:
    ePaperPort(ImgDecodeDither &dither,int mosi, int scl, int dc, int csm, int css, int rst, int busy, uint16_t width, uint16_t height, uint16_t scale_MaxWidth, uint16_t scale_MaxHeight, spi_host_device_t spihost = SPI3_HOST);
    ~ePaperPort();

    void EPD_Init();
    void EPD_DispClear(uint8_t color);
    void EPD_Display();
    void EPD_Sleep();
    void EPD_SrcDisplayCopy(uint8_t *buffer,uint32_t len,uint32_t addlen);
    void Set_Rotation(uint8_t rot); // 0:no 1:90 2:180 3:270
    void Set_Mirror(uint8_t mirr_x,uint8_t mirr_y);
    uint8_t* EPD_GetIMGBuffer();
    void EPD_SetPixel(uint32_t x, uint32_t y, uint8_t color);
    void EPD_SetBMPPixel(uint32_t x, uint32_t y, uint8_t color);
    void EPD_SDcardBmpShakingColor(const char *path,uint16_t x_start, uint16_t y_start);        /*只能用于经过抖动之后的 480x800/800x480 BMP图片显示*/
    void EPD_ImgBufferToDispBuffer(uint8_t *buffer, uint16_t width, uint16_t height);        /*直接使用图片解码后的缓存区进行处理*/
    void EPD_SDcardIMGShakingColor(const char *path,uint16_t x_start, uint16_t y_start);        /*可以显示jpg,bmp,png格式图片 480x800/800x480*/
    void EPD_SDcardScaleIMGShakingColor(const char *path,uint16_t x_start, uint16_t y_start);   /*可以显示jpg,bmp,png格式图片,带自动拉伸缩放的*/
    void EPD_SDcardScaleIMGShakingColor_NotSave(const char *path);    /*可以显示jpg,bmp,png格式图片,带自动拉伸缩放的,但是不将处理好后的图片存储到 TF 卡中*/
	void EPD_DrawStringCN(uint16_t Xstart, uint16_t Ystart, const char * pString, cFONT* font,uint16_t Color_Foreground, uint16_t Color_Background);
    void EPD_DrawStringEN(uint16_t Xstart, uint16_t Ystart, const char * pString,sFONT* Font, uint16_t Color_Foreground, uint16_t Color_Background);
    void EPD_BmpSrcBuffer_flee();
    uint8_t* EPD_DispBuffer();
    uint8_t* EPD_RotationBuffer();
    uint8_t* EPD_BmpSrcBuffer();
    void EPD_Rotate180_Fast(const uint8_t* src, uint8_t* dst, int width, int height);
    void EPD_Rotate90CCW_Fast(const uint8_t* src, uint8_t* dst, int width, int height);
    void EPD_Rotate90CW_Fast(const uint8_t* src, uint8_t* dst, int width, int height);
};