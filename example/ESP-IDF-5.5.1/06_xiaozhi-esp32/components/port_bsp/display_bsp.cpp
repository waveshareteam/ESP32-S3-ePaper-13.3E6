#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "display_bsp.h"
#include "esp_task_wdt.h"

#include "esp_heap_caps.h"


// Define the data cache area of the e-ink screen
uint8_t *Image_Temporary;

const uint8_t PSR_V[2] = {
	0xDF, 0x69
};
const uint8_t PWR_V[6] = {
	0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38
};
const uint8_t POF_V[1] = {
	0x00
};
const uint8_t DRF_V[1] = {
	0x00
};
const uint8_t CDI_V[1] = {
	0xF7
};
const uint8_t TCON_V[2] = {
	0x03, 0x03
};
const uint8_t TRES_V[4] = {
	0x04, 0xB0, 0x03, 0x20
};
const uint8_t CMD66_V[6] = {
	0x49, 0x55, 0x13, 0x5D, 0x05, 0x10
};
const uint8_t EN_BUF_V[1] = {
	0x07
};
const uint8_t CCSET_V[1] = {
	0x01
};
const uint8_t PWS_V[1] = {
	0x22
};
const uint8_t AN_TM_V[9] = {
	0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55
};
const uint8_t AGID_V[1] = {
	0x10
};
const uint8_t BTST_P_V[2] = {
	0xE8, 0x28
};
const uint8_t BOOST_VDDP_EN_V[1] = {
	0x01
};
const uint8_t BTST_N_V[2] = {
	0xE8, 0x28
};
const uint8_t BUCK_BOOST_VDDN_V[1] = {
	0x01
};
const uint8_t TFT_VCOM_POWER_V[1] = {
	0x02
};

ePaperPort::ePaperPort(ImgDecodeDither &dither,int mosi, int scl, int dc, int csm, int css, int rst, int busy, uint16_t width, uint16_t height,uint16_t scale_MaxWidth, uint16_t scale_MaxHeight, spi_host_device_t spihost) : 
dither_(dither),
mosi_(mosi), 
scl_(scl), 
dc_(dc), 
csm_(csm), 
css_(css), 
rst_(rst), 
busy_(busy), 
width_(width), 
height_(height),
scale_MaxWidth_(scale_MaxWidth),
scale_MaxHeight_(scale_MaxHeight) {
    esp_err_t        ret;
    spi_bus_config_t buscfg   = {};
    uint32_t              transfer = width_ * height_;
    DisplayLen                = transfer / 2; //(1byte 2ipex)
    DispBuffer                = (uint8_t *)heap_caps_malloc(DisplayLen, MALLOC_CAP_SPIRAM);
    assert(DispBuffer);
    RotationBuffer             = (uint8_t *)heap_caps_malloc(DisplayLen, MALLOC_CAP_SPIRAM);
    assert(RotationBuffer);
    // BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(width_ * height_ * 6 + 2000, MALLOC_CAP_SPIRAM); 
    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM);  //11.5M
    assert(BmpSrcBuffer);
    buscfg.miso_io_num                   = -1;
    buscfg.mosi_io_num                   = mosi;
    buscfg.sclk_io_num                   = scl;
    buscfg.quadwp_io_num                 = -1;
    buscfg.quadhd_io_num                 = -1;
    buscfg.max_transfer_sz               = transfer;
    spi_device_interface_config_t devcfg = {};
    devcfg.spics_io_num                  = -1;
    devcfg.clock_speed_hz                = 10 * 1000 * 1000;    // Clock out at 10 MHz
    devcfg.mode                          = 0;                   // SPI mode 0
    devcfg.queue_size                    = 7;                   // We want to be able to queue 7 transactions at a time
    devcfg.flags                         = SPI_DEVICE_HALFDUPLEX;
    ret                                  = spi_bus_initialize(spihost, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(spihost, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask  = (0x1ULL << rst_) | (0x1ULL << dc_) | (0x1ULL << csm_) | (0x1ULL << css_);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    gpio_conf.intr_type    = GPIO_INTR_DISABLE;
    gpio_conf.mode         = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask = (0x1ULL << busy_);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    Set_ResetIOLevel(1);
}

ePaperPort::~ePaperPort() {
}

void ePaperPort::Set_ResetIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) rst_, level ? 1 : 0);
}

void ePaperPort::Set_CSMIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) csm_, level ? 1 : 0);
}

void ePaperPort::Set_CSSIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) css_, level ? 1 : 0);
}

void ePaperPort::Set_CSALLIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) csm_, level ? 1 : 0);
    gpio_set_level((gpio_num_t) css_, level ? 1 : 0);
}

void ePaperPort::Set_DCIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) dc_, level ? 1 : 0);
}

uint8_t ePaperPort::Get_BusyIOLevel() {
    return gpio_get_level((gpio_num_t) busy_);
}

void ePaperPort::EPD_Reset(void) {
    Set_ResetIOLevel(1);
    vTaskDelay(pdMS_TO_TICKS(50));
    Set_ResetIOLevel(0);
    vTaskDelay(pdMS_TO_TICKS(20));
    Set_ResetIOLevel(1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void ePaperPort::EPD_LoopBusy(void) {
    while (1) {
        if (Get_BusyIOLevel()) {
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ePaperPort::SPI_Write(uint8_t data) {
    esp_err_t         ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length    = 8;
    t.tx_buffer = &data;
    ret         = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void ePaperPort::EPD_SendCommand(uint8_t Reg) {
    // Set_DCIOLevel(0);
    SPI_Write(Reg);
}

void ePaperPort::EPD_SendData(uint8_t Data) {
    // Set_DCIOLevel(1);
    SPI_Write(Data);
}

void ePaperPort::EPD_Sendbuffera(const uint8_t *Data, uint32_t len) {
    // Set_DCIOLevel(1);

    esp_err_t ret;
    const size_t chunk_size = 4096;
    size_t i = 0;
    
    for (i = 0; i < len; i += chunk_size) {
        size_t current_chunk = (i + chunk_size > len) ? (len - i) : chunk_size;
        
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        
        t.length = current_chunk * 8;
        t.rxlength = 0;   
        t.tx_buffer = Data + i;
        t.rx_buffer = NULL;      
        
        ret = spi_device_polling_transmit(spi, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmission failed: %s", esp_err_to_name(ret));
            return;
        }
    }
    
    // ESP_LOGW(TAG, "All %ld bytes transmitted successfully", len);
    // ESP_LOGW(TAG, "All %ld bytes transmitted successfully", i);

   
    // assert(ret == ESP_OK);
}

void ePaperPort::EPD_SPI_Send(uint8_t Cmd, const uint8_t *buf, uint32_t Len)
{
    EPD_SendCommand(Cmd);
    EPD_Sendbuffera(buf,Len);
}

void ePaperPort::EPD_TurnOnDisplay(void) {

    ESP_LOGI(TAG,"Write PON");
    Set_CSALLIOLevel(0);
    EPD_SendCommand(0x04); // POWER_ON
    Set_CSALLIOLevel(1);
    EPD_LoopBusy();

    ESP_LOGI(TAG,"Write DRF");
    vTaskDelay(pdMS_TO_TICKS(50));

    Set_CSALLIOLevel(0);
    EPD_SPI_Send(DRF, DRF_V, sizeof(DRF_V));
    Set_CSALLIOLevel(1);
    EPD_LoopBusy();

    ESP_LOGI(TAG,"Write POF");
    Set_CSALLIOLevel(0);
    EPD_SPI_Send(POF, POF_V, sizeof(POF_V));
    Set_CSALLIOLevel(1);
    EPD_LoopBusy();
    ESP_LOGI(TAG,"Display Done!!");
}

void ePaperPort::Set_Rotation(uint8_t rot) {
    Rotation = rot;
}

void ePaperPort::Set_Mirror(uint8_t mirr_x,uint8_t mirr_y) {
    mirrx = mirr_x;
    mirry = mirr_y;
}

void ePaperPort::EPD_Init() {
    // if(isEPDInit) {
    //     ESP_LOGW(TAG, "EPD has already been initialized.");
    //     return;
    // }
    ESP_LOGE(TAG,"EPD_Init");
    EPD_Reset();
    EPD_LoopBusy();
    vTaskDelay(pdMS_TO_TICKS(50));

    Set_CSMIOLevel(0);
	EPD_SPI_Send(AN_TM, AN_TM_V, sizeof(AN_TM_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(CMD66, CMD66_V, sizeof(CMD66_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(PSR, PSR_V, sizeof(PSR_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(CDI, CDI_V, sizeof(CDI_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(TCON, TCON_V, sizeof(TCON_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(AGID, AGID_V, sizeof(AGID_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(PWS, PWS_V, sizeof(PWS_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(CCSET, CCSET_V, sizeof(CCSET_V));
    Set_CSALLIOLevel(1);

    Set_CSALLIOLevel(0);
	EPD_SPI_Send(TRES, TRES_V, sizeof(TRES_V));
    Set_CSALLIOLevel(1);

    Set_CSMIOLevel(0);
	EPD_SPI_Send(PWR, PWR_V, sizeof(PWR_V));
    Set_CSALLIOLevel(1);

    Set_CSMIOLevel(0);
	EPD_SPI_Send(EN_BUF, EN_BUF_V, sizeof(EN_BUF_V));
    Set_CSALLIOLevel(1);

    Set_CSMIOLevel(0);
	EPD_SPI_Send(BTST_P, BTST_P_V, sizeof(BTST_P_V));
    Set_CSALLIOLevel(1);

    Set_CSMIOLevel(0);
	EPD_SPI_Send(BOOST_VDDP_EN, BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
    Set_CSALLIOLevel(1);

    Set_CSMIOLevel(0);
	EPD_SPI_Send(BTST_N, BTST_N_V, sizeof(BTST_N_V));
    Set_CSALLIOLevel(1);

    Set_CSMIOLevel(0);
	EPD_SPI_Send(BUCK_BOOST_VDDN, BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
    Set_CSALLIOLevel(1);

    Set_CSMIOLevel(0);
	EPD_SPI_Send(TFT_VCOM_POWER, TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
    Set_CSALLIOLevel(1);

    // EPD_DispClear(ColorWhite);
    // isEPDInit = true;
}

void ePaperPort::EPD_DispClear(uint8_t color) {
    uint8_t *buffer = DispBuffer;
    for (int j = 0; j < DisplayLen; j++) {
        buffer[j] = (color << 4) | color;
    }
}

void ePaperPort::EPD_Display() {
    // EPD_PixelRotate();

    size_t buffer_size =  width_ * height_ / 4;
    if((Image_Temporary = (uint8_t *)heap_caps_malloc(buffer_size,MALLOC_CAP_SPIRAM)) == NULL) 
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        // return ESP_FAIL;
        uint32_t Width, Width1, Height;
        Width = width_ / 2;
        Width1 = width_ / 4;
        Height = height_;

        ESP_LOGE(TAG,"CS_M_0");
        Set_CSMIOLevel(0);
        EPD_SendCommand(0x10);
        for(uint32_t i=0; i<Height; i++ )
        {
            EPD_Sendbuffera(DispBuffer + i*Width,Width1);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
            
        Set_CSALLIOLevel(1);

        ESP_LOGE(TAG,"CS_S_0");
        Set_CSSIOLevel(0);
        EPD_SendCommand(0x10);
        for(uint32_t i=0; i<Height; i++ )
        {
            EPD_Sendbuffera(DispBuffer + i*Width + Width1,Width1);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        Set_CSALLIOLevel(1);

    } else {

        ESP_LOGE(TAG,"111111");

        uint32_t Width = width_ / 4;
        uint32_t Height = height_;
        for (uint32_t i = 0; i < Height; i++)
        {
            for (uint32_t j = 0; j < Width; j++)
            {
                Image_Temporary[i*Width+j] = DispBuffer[i*Width*2+j];
            }
        }
        ESP_LOGE(TAG,"CS_M_0");
        Set_CSMIOLevel(0);
        EPD_SendCommand(0x10);
        EPD_Sendbuffera(Image_Temporary, buffer_size);
        Set_CSALLIOLevel(1);

        for (uint32_t i = 0; i < Height; i++)
        {
            for (uint32_t j = 0; j < Width; j++)
            {
                Image_Temporary[i*Width+j] = DispBuffer[i*Width*2+j+Width];
            }
        }
        ESP_LOGE(TAG,"CS_S_0");
        Set_CSSIOLevel(0);
        EPD_SendCommand(0x10);
        EPD_Sendbuffera(Image_Temporary, buffer_size);
        Set_CSALLIOLevel(1);

        heap_caps_free(Image_Temporary);
    }
    EPD_TurnOnDisplay();
}

void ePaperPort::EPD_Sleep()
{
    Set_CSALLIOLevel(0);
    EPD_SendCommand(0x07); // DEEP_SLEEP
    EPD_SendData(0XA5);
    Set_CSALLIOLevel(1);

    // Turn off the power
    vTaskDelay(pdMS_TO_TICKS(100));
}

void ePaperPort::EPD_SrcDisplayCopy(uint8_t *buffer,uint32_t len,uint32_t addlen) {
    if((addlen + len) > DisplayLen) {
        ESP_LOGE(TAG,"Data exceeds the buffer area.");
        return;
    }
    for(uint32_t i = 0; i < len; i++) {
        RotationBuffer[addlen + i] = buffer[i];
    }
    ESP_LOGW(TAG,"buffer: %d",addlen + len);
}

uint8_t* ePaperPort::EPD_GetIMGBuffer() {
    return DispBuffer;
}

void ePaperPort::EPD_SetPixel(uint32_t x, uint32_t y, uint8_t color) {
    // if(x >= 1200 || y >= 1600) {
    //     ESP_LOGE("Pixel","Beyond the limit: (%d,%d)",x,y);
    //     return;
    // }
        // uint32_t index = (y << 8) + (y << 7) + (y << 4) + (x >> 1);
    uint32_t index = (y*width_ + x)/2;
    // uint8_t Rdata = DispBuffer[index];
    if(x%2 == 0){
        DispBuffer[index] = DispBuffer[index] & 0xF0;
        DispBuffer[index] = DispBuffer[index] | color;
    } else {
        DispBuffer[index] = DispBuffer[index] & 0x0F;
        DispBuffer[index] = DispBuffer[index] | (color << 4);
    }
  
    // Rdata = Rdata & (~(0xF0 >> ((x % 2)*4)));//Clear first, then set value
    // DispBuffer[index] = Rdata | ((color << 4) >> ((x % 2)*4));

    // printf("%ld,%x, ", index, DispBuffer[index]);
}

void ePaperPort::EPD_SetBMPPixel(uint32_t x, uint32_t y, uint8_t color) {
    // if(x >= 1200 || y >= 1600) {
    //     ESP_LOGE("Pixel","Beyond the limit: (%d,%d)",x,y);
    //     return;
    // }
        // uint32_t index = (y << 8) + (y << 7) + (y << 4) + (x >> 1);
    uint32_t index = (y*height_ + x)/2;
    // uint8_t Rdata = DispBuffer[index];
    if(x%2 == 0){
        DispBuffer[index] = DispBuffer[index] & 0xF0;
        DispBuffer[index] = DispBuffer[index] | color;
    } else {
        DispBuffer[index] = DispBuffer[index] & 0x0F;
        DispBuffer[index] = DispBuffer[index] | (color << 4);
    }
  
    // Rdata = Rdata & (~(0xF0 >> ((x % 2)*4)));//Clear first, then set value
    // DispBuffer[index] = Rdata | ((color << 4) >> ((x % 2)*4));

    // printf("%ld,%x, ", index, DispBuffer[index]);
}

uint8_t* ePaperPort::EPD_ParseBMPImage(const char *path) {
    FILE *fp;
    if ((fp = fopen(path, "rb")) == NULL) {
        ESP_LOGE(TAG, "Cann't open the file!");
        return NULL;
    }
    BMPFILEHEADER bmpFileHeader;
    BMPINFOHEADER bmpInfoHeader;

    fseek(fp, 0, SEEK_SET);
    fread(&bmpFileHeader, sizeof(BMPFILEHEADER), 1, fp);  
    fread(&bmpInfoHeader, sizeof(BMPINFOHEADER), 1, fp);

    ESP_LOGW(TAG, "(WIDTH:HEIGHT) = (%ld:%ld)", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);
    src_width  = bmpInfoHeader.biWidth;
    src_height = bmpInfoHeader.biHeight;
    int readbyte = bmpInfoHeader.biBitCount;
    if (readbyte != 24) {
        ESP_LOGE(TAG, "Bmp image is not 24 bitmap!");
        fclose(fp);
        return NULL;
    }

    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); 
    assert(BmpSrcBuffer);

    fseek(fp, bmpFileHeader.bOffset, SEEK_SET);
    int rowBytes = src_width * 3;
    for (int y = src_height - 1; y >= 0; y--) {
        uint8_t *rowPtr = BmpSrcBuffer + y * rowBytes;
        fread(rowPtr, 1, rowBytes, fp);
        // Delay feeding the watchdog
        // esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1));  
    }
    fclose(fp);
    if(src_width == 1600) {
        Rotation = 3;
        // src_width = 1200;
        // src_height = 1600;
    } else {
        Rotation = 2;
    }
    return BmpSrcBuffer;
}

uint8_t ePaperPort::EPD_ColorToePaperColor(uint8_t b,uint8_t g,uint8_t r) {
    if(b == 0xff && g == 0xff && r == 0xff) {
        return ColorWhite;
    }
    if(b == 0x0 && g == 0x0 && r == 0x0) {
        return ColorBlack;
    }
    if(b == 0x0 && g == 0x0 && r == 0xff) {
        return ColorRed;
    }
    if(b == 0xff && g == 0x0 && r == 0x0) {
        return ColorBlue;
    }
    if(b == 0x0 && g == 0xff && r == 0x0) {
        return ColorGreen;
    }
    if(b == 0x0 && g == 0xff && r == 0xff) {
        return ColorYellow;
    }
    return ColorWhite;
}

void ePaperPort::EPD_SDcardBmpShakingColor(const char *path,uint16_t x_start, uint16_t y_start) {
    uint8_t r,g,b;
    uint8_t color;
    uint32_t idx;
    uint8_t *buffer = EPD_ParseBMPImage(path);
    // uint8_t *buffer = BmpSrcBuffer;
    if(NULL == buffer) {
        return;
    }
    // uint8_t* scapeBuffer = (uint8_t*)buffer;
    
    ESP_LOGE("EPD_SDcardBmpShakingColor", "src_width = %ld , src_height = %ld", src_width, src_height);
    if(Rotation == 0 || Rotation == 2){
        ESP_LOGW("Rotate","Rotate0 or Rotate180");
        for(uint32_t y = 0; y < src_height; y++) {
            for(uint32_t x = 0; x < src_width; x++) {
                idx = (y * src_width + x) * 3;
                b = buffer[idx + 0];
                g = buffer[idx + 1];
                r = buffer[idx + 2];
                color = EPD_ColorToePaperColor(b, g, r);
                // printf("0x%x, ",color);
                EPD_SetPixel(x_start + x, y_start + y, color);
            }
            // Delay feeding the watchdog
            // esp_task_wdt_reset();
            // vTaskDelay(pdMS_TO_TICKS(1));  
        }
    } else {
        ESP_LOGW("Rotate","Rotate90 or Rotate270");
        for(uint32_t y = 0; y < src_height; y++) {
            for(uint32_t x = 0; x < src_width; x++) {
                idx = (y * src_width + x) * 3;
                b = buffer[idx + 0];
                g = buffer[idx + 1]; 
                r = buffer[idx + 2];
                color = EPD_ColorToePaperColor(b, g, r);
                EPD_SetPixel(x_start + (src_height - 1 - y), y_start + x, color);
                // EPD_SetPixel(x_start + y, y_start + (src_width - x - 1), color);
                // EPD_SetBMPPixel(x_start + y, y_start + (src_width - x - 1), color);
            }
            // Delay feeding the watchdog
            // esp_task_wdt_reset();
            // vTaskDelay(pdMS_TO_TICKS(1));  
        }
    }

    free(buffer);
    buffer = NULL;
    
}

void ePaperPort::EPD_ImgBufferToDispBuffer(uint8_t *buffer, uint16_t width, uint16_t height) {
    uint8_t r,g,b;
    uint8_t color;
    uint32_t idx;
    // uint8_t *buffer = BmpSrcBuffer;
    if(NULL == buffer) {
        return;
    }
    // uint8_t* scapeBuffer = (uint8_t*)buffer;

    if(width == 1600) {
        Rotation = 3;
        // src_width = 1200;
        // src_height = 1600;
    } else {
        Rotation = 2;
    }
    
    ESP_LOGE("EPD_SDcardBmpShakingColor", "src_width = %ld , src_height = %ld", width, height);
    if(Rotation == 0 || Rotation == 2){
        ESP_LOGW("Rotate","Rotate0 or Rotate180");
        for(uint32_t y = 0; y < height; y++) {
            for(uint32_t x = 0; x < width; x++) {
                idx = (y * width + x) * 3;
                b = buffer[idx + 0];
                g = buffer[idx + 1];
                r = buffer[idx + 2];
                color = EPD_ColorToePaperColor(b, g, r);
                // printf("0x%x, ",color);
                EPD_SetPixel(x, y, color);
            }
            // Delay feeding the watchdog
            // esp_task_wdt_reset();
            // vTaskDelay(pdMS_TO_TICKS(1));  
        }
    } else {
        ESP_LOGW("Rotate","Rotate90 or Rotate270");
        for(uint32_t y = 0; y < height; y++) {
            for(uint32_t x = 0; x < width; x++) {
                idx = (y * width + x) * 3;
                b = buffer[idx + 0];
                g = buffer[idx + 1]; 
                r = buffer[idx + 2];
                color = EPD_ColorToePaperColor(b, g, r);
                EPD_SetPixel(height - 1 - y, x, color);
                // EPD_SetPixel(x_start + y, y_start + (width - x - 1), color);
                // EPD_SetBMPPixel(x_start + y, y_start + (width - x - 1), color);
            }
            // Delay feeding the watchdog
            // esp_task_wdt_reset();
            // vTaskDelay(pdMS_TO_TICKS(1));  
        }
    }
}


void ePaperPort::EPD_SDcardIMGShakingColor(const char *path,uint16_t x_start, uint16_t y_start) {
    uint8_t *decimgbuff = NULL;
    int img_len = 0;
    uint8_t *floyd_buffer = NULL;
    int s_width;
    int s_height;

    if(BmpSrcBuffer != NULL){
        printf("111111111\r\n");
        free(BmpSrcBuffer);
        BmpSrcBuffer = NULL;
    }

    size_t total;
    size_t freel;
    size_t largest_block;

    total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("PSRAM1111", "S3-N32R16 PSRAM: Total=%dKB (%.2fMB), Free=%dKB", total/1024, (float)total/(1024*1024), freel/1024);

    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("MEM", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);

    if(strstr(path, ".jpg") || strstr(path, ".JPG") || strstr(path, ".jpeg") || strstr(path, ".JPEG")) {
        if(dither_.ImgDecode_TFOneJPGPicture(path,&decimgbuff,&img_len,&s_width,&s_height) == ESP_OK) {
            floyd_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM);       // Store the data after applying the RGB888 jitter algorithm
            // assert(floyd_buffer);
            ESP_LOGW(TAG,"jpgdecode:(%d,%d)",s_width,s_height);
            if(floyd_buffer == NULL)
            {
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, decimgbuff, s_width,s_height) == ESP_OK) {
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                    ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                } else {
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                }
                /* code */
            } else {
                dither_.ImgDecode_DitherRgb888(decimgbuff, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                dither_.ImgDecode_JPGBufferFree(decimgbuff); 
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                    ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                }
            }
        } else {
            ESP_LOGE(TAG, "jpg dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_JPGBufferFree(decimgbuff);
            }
        }
    } else if(strstr(path, ".png") || strstr(path, ".PNG")) {
        if(dither_.ImgDecode_TFOnePNGPicture(path,&decimgbuff,&s_width,&s_height) == ESP_OK) {
            floyd_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM);       // Store the data after applying the RGB888 jitter algorithm
            // assert(floyd_buffer);
            ESP_LOGW(TAG,"pngdecode:(%d,%d)",s_width,s_height);
            if(floyd_buffer == NULL)
            {
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, decimgbuff, s_width,s_height) == ESP_OK) {
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                    ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                } else {
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                }
                /* code */
            } else {
                dither_.ImgDecode_DitherRgb888(decimgbuff, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                dither_.ImgDecode_JPGBufferFree(decimgbuff); 
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                    ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                }
            }
        } else {
            ESP_LOGE(TAG, "PNG dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_PNGBufferFree(decimgbuff);
            }
        }
    } else if(strstr(path, ".bmp") || strstr(path, ".BMP")) {
        if(dither_.ImgDecodebmp_TFOneBMPPicture(path,&decimgbuff,&s_width,&s_height) == ESP_OK) {
            floyd_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM);       // Store the data after applying the RGB888 jitter algorithm
            // assert(floyd_buffer);
            ESP_LOGW(TAG,"bmpdecode:(%d,%d)",s_width,s_height);
            if(floyd_buffer == NULL) {
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, decimgbuff, s_width,s_height) == ESP_OK) {
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                    ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                } else {
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                }
                /* code */
            } else {
                dither_.ImgDecode_DitherRgb888(decimgbuff, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                dither_.ImgDecode_JPGBufferFree(decimgbuff); 
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                    ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                }
            }
        } else {
            ESP_LOGE(TAG, "BMP dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_BMPBufferFree(decimgbuff);
            }
        }
    }

    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("MEM-end", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

    // PSRAM recycling prevents other programs from disrupting continuity
    if(largest_block < 23 * 512 * 1024){
        BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
    } else {
        
        BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
    }
    assert(BmpSrcBuffer);
    
    // // PSRAM recycling prevents other programs from disrupting continuity
    // BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
    // assert(BmpSrcBuffer);
}
void ePaperPort::EPD_SDcardScaleIMGShakingColor(const char *path,uint16_t x_start, uint16_t y_start) {
    uint8_t *decimgbuff = NULL;
    int img_len = 0;
    int s_width;
    int s_height;

    if(BmpSrcBuffer != NULL){
        printf("111111111\r\n");
        free(BmpSrcBuffer);
        BmpSrcBuffer = NULL;
    }

    uint8_t *scale_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
    assert(scale_buffer);

    uint8_t *floyd_buffer = NULL;


    size_t total;
    size_t freel;
    size_t largest_block;

    total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("PSRAM1111", "S3-N32R16 PSRAM: Total=%dKB (%.2fMB), Free=%dKB", total/1024, (float)total/(1024*1024), freel/1024);

    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("MEM", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);
    
    if(strstr(path, ".jpg") || strstr(path, ".JPG") || strstr(path, ".jpeg") || strstr(path, ".JPEG") ) {
        free(scale_buffer);
        if(dither_.ImgDecode_TFOneJPGPicture(path,&decimgbuff,&img_len,&s_width,&s_height) == ESP_OK) {
            ESP_LOGW(TAG,"jpgdecode:(%d,%d)",s_width,s_height);
            if((s_width > 1600 || s_height > 1200) && (s_width > 1200 || s_height > 1600)) {
                dither_.ImgDecode_JPGBufferFree(decimgbuff); 

                // PSRAM recycling prevents other programs from disrupting continuity
                if(largest_block < 23 * 512 * 1024){
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
                } else {
                    
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
                }
                assert(BmpSrcBuffer);

                return;
            }
            if((1600 == s_width && 1200 == s_height) || (1200 == s_width && 1600 == s_height)) {

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-jpg0", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);

                floyd_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
                // assert(floyd_buffer);
                if(floyd_buffer == NULL)
                {
                    if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, decimgbuff, s_width,s_height) == ESP_OK) {
                        dither_.ImgDecode_JPGBufferFree(decimgbuff);
                        ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        dither_.ImgDecode_JPGBufferFree(decimgbuff);
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                    }
                    /* code */
                } else {
                    dither_.ImgDecode_DitherRgb888(decimgbuff, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                    dither_.ImgDecode_JPGBufferFree(decimgbuff); 
                    if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                        ESP_LOGE("SD1", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                    }
                }
            } else { /*拉伸缩放*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-jpg1", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);
                
                scale_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
                assert(scale_buffer);

                if(s_width < s_height) {
                    ESP_LOGE("picture", "portrait screen s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,width_,height_);
                    s_width = width_;
                    s_height = height_;
                } else {
                    ESP_LOGE("picture", "landscape s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,height_,width_);
                    s_width = height_;
                    s_height = width_;
                }
                dither_.ImgDecode_JPGBufferFree(decimgbuff);

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-jpg2", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);


                floyd_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM);       // Store the data after applying the RGB888 jitter algorithm
                // assert(floyd_buffer);
                if(floyd_buffer == NULL) {
                    if (dither_.ImgDecode_DitherRgb888ToSdcard(img_to_bmpName, scale_buffer, s_width,s_height) == ESP_OK) {
                        free(scale_buffer);
                        scale_buffer = NULL;
                        ESP_LOGE("SD2", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                        free(scale_buffer);
                        scale_buffer = NULL;
                    }
                } else {
                    dither_.ImgDecode_DitherRgb888(scale_buffer, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                    free(scale_buffer);
                    scale_buffer = NULL;
                    if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                        ESP_LOGE("SD2", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                    }
                }
                
            }
        } else { /*解码失败*/
            ESP_LOGE(TAG, "jpg dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_JPGBufferFree(decimgbuff);
            }
        }
    } else if(strstr(path, ".png") || strstr(path, ".PNG")) {
        if(dither_.ImgDecode_TFOnePNGPicture(path,&decimgbuff,&s_width,&s_height) == ESP_OK) {
            if((s_width > 1600 || s_height > 1200) && (s_width > 1200 || s_height > 1600)) {
                dither_.ImgDecode_PNGBufferFree(decimgbuff);
                free(scale_buffer);
                scale_buffer = NULL;
                // PSRAM recycling prevents other programs from disrupting continuity
                if(largest_block < 23 * 512 * 1024){
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
                } else {
                    
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
                }
                assert(BmpSrcBuffer);

                return;
            }
            if((1600 == s_width && 1200 == s_height) || (1200 == s_width && 1600 == s_height)) { /*正常抖动显示*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-png0", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);

                floyd_buffer = scale_buffer;       // Store the data after applying the RGB888 jitter algorithm
                dither_.ImgDecode_DitherRgb888(decimgbuff, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                dither_.ImgDecode_JPGBufferFree(decimgbuff); 
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                    ESP_LOGE("SD3", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                }
            } else {   /*拉伸缩放*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-png1", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);

                if(s_width < s_height) {
                    ESP_LOGE("picture", "portrait screen s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,width_,height_);
                    s_width = width_;
                    s_height = height_;
                } else {
                    ESP_LOGE("picture", "landscape s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,height_,width_);
                    s_width = height_;
                    s_height = width_;
                }
                dither_.ImgDecode_PNGBufferFree(decimgbuff);

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-png2", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);

                floyd_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM);       // Store the data after applying the RGB888 jitter algorithm
                // assert(floyd_buffer);
                if(floyd_buffer == NULL) {
                    if (dither_.ImgDecode_DitherRgb888ToSdcard(img_to_bmpName, scale_buffer, s_width,s_height) == ESP_OK) {
                        free(scale_buffer);
                        scale_buffer = NULL;
                        ESP_LOGE("SD2", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                        free(scale_buffer);
                        scale_buffer = NULL;
                    }
                } else {
                    dither_.ImgDecode_DitherRgb888(scale_buffer, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                    free(scale_buffer);
                    scale_buffer = NULL;
                    if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                        ESP_LOGE("SD4", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                    }
                }
            }
        } else {      /*解码失败*/
            ESP_LOGE(TAG, "PNG dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_PNGBufferFree(decimgbuff);
            }
        }
    } else if(strstr(path, ".bmp") || strstr(path, ".BMP")) {
        if(dither_.ImgDecodebmp_TFOneBMPPicture(path,&decimgbuff,&s_width,&s_height) == ESP_OK) {
            if((s_width > 1600 || s_height > 1200) && (s_width > 1200 || s_height > 1600)) {
                ESP_LOGE("width、height", "out of scope");
                dither_.ImgDecode_BMPBufferFree(decimgbuff);
                free(scale_buffer);
                scale_buffer = NULL;
                // PSRAM recycling prevents other programs from disrupting continuity
                if(largest_block < 23 * 512 * 1024){
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
                } else {
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
                }
                assert(BmpSrcBuffer);

                return;
            }
            if((1600 == s_width && 1200 == s_height) || (1200 == s_width && 1600 == s_height)) { /*正常抖动显示*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-bmp0", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

                floyd_buffer = scale_buffer;       // Store the data after applying the RGB888 jitter algorithm
                dither_.ImgDecode_DitherRgb888(decimgbuff, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                dither_.ImgDecode_JPGBufferFree(decimgbuff); 
                if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                    ESP_LOGE("SD5", "img_to_bmpName = %s", img_to_bmpName);
                    EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    // for (size_t i = 0; i < 1600; i++)
                    // {
                    //     for (size_t j = 0; j < 1200; j++)
                    //     {
                    //         printf("0x%x, ",DispBuffer[i*1200 + j]);
                    //     }
                    //     printf("\r\n");
                    // }
                    
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(floyd_buffer);
                    floyd_buffer = NULL;
                }
            } else {   /*拉伸缩放*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-bmp1", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

                if(s_width < s_height) {
                    ESP_LOGE("picture", "portrait screen s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,width_,height_);
                    s_width = width_;
                    s_height = height_;
                } else {
                    ESP_LOGE("picture", "landscape s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,height_,width_);
                    s_width = height_;
                    s_height = width_;
                }
                dither_.ImgDecode_BMPBufferFree(decimgbuff);
                
                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-bmp2", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);

                floyd_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM);       // Store the data after applying the RGB888 jitter algorithm
                // assert(floyd_buffer);
                if(floyd_buffer == NULL) {
                    if (dither_.ImgDecode_DitherRgb888ToSdcard(img_to_bmpName, scale_buffer, s_width,s_height) == ESP_OK) {
                        free(scale_buffer);
                        scale_buffer = NULL;
                        ESP_LOGE("SD2", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                        free(scale_buffer);
                        scale_buffer = NULL;
                    }
                } else {
                    dither_.ImgDecode_DitherRgb888(scale_buffer, floyd_buffer,s_width,s_height);     //The RGB888 data has undergone the jittering algorithm.
                    free(scale_buffer);
                    scale_buffer = NULL;
                    if (dither_.ImgDecode_EncodingBmpToSdcard(img_to_bmpName, floyd_buffer, s_width,s_height) == ESP_OK) {
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                        ESP_LOGE("SD6", "img_to_bmpName = %s", img_to_bmpName);
                        EPD_SDcardBmpShakingColor(img_to_bmpName,x_start,y_start);
                    } else {
                        ESP_LOGE(TAG, "bmp to sdcard fill");
                        free(floyd_buffer);
                        floyd_buffer = NULL;
                    }
                }
            }
        } else {
            ESP_LOGE(TAG, "BMP dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_BMPBufferFree(decimgbuff);
            }
        }
    }

    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("MEM-end", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

    // PSRAM recycling prevents other programs from disrupting continuity
    if(largest_block < 23 * 512 * 1024){
        BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
    } else {
        
        BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
    }
    assert(BmpSrcBuffer);
}

void ePaperPort::EPD_SDcardScaleIMGShakingColor_NotSave(const char *path) {
    uint8_t *decimgbuff = NULL;
    int img_len = 0;
    int s_width;
    int s_height;

    if(BmpSrcBuffer != NULL){
        printf("111111111\r\n");
        free(BmpSrcBuffer);
        BmpSrcBuffer = NULL;
    }

    uint8_t *scale_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
    assert(scale_buffer);

    uint8_t *floyd_buffer = NULL;
    size_t total;
    size_t freel;
    size_t largest_block;

    total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("PSRAM1111", "S3-N32R16 PSRAM: Total=%dKB (%.2fMB), Free=%dKB", total/1024, (float)total/(1024*1024), freel/1024);

    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("MEM", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);
    
    if(strstr(path, ".jpg") || strstr(path, ".JPG") || strstr(path, ".jpeg") || strstr(path, ".JPEG") ) {
        free(scale_buffer);
        scale_buffer = NULL;
        if(dither_.ImgDecode_TFOneJPGPicture(path,&decimgbuff,&img_len,&s_width,&s_height) == ESP_OK) {
            ESP_LOGW(TAG,"jpgdecode:(%d,%d)",s_width,s_height);
            if((s_width > 1600 || s_height > 1200) && (s_width > 1200 || s_height > 1600)) {
                dither_.ImgDecode_JPGBufferFree(decimgbuff); 

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-end", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);
                // PSRAM recycling prevents other programs from disrupting continuity
                if(largest_block < 23 * 512 * 1024){
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
                } else {
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
                }
                assert(BmpSrcBuffer);

                return;
            }
            if((1600 == s_width && 1200 == s_height) || (1200 == s_width && 1600 == s_height)) {
                if (dither_.ImgDecode_DitherRgb888ToOriginalBuffer(decimgbuff, s_width,s_height) == ESP_OK) {
                    EPD_ImgBufferToDispBuffer(decimgbuff, s_width,s_height);
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                } else {
                    dither_.ImgDecode_JPGBufferFree(decimgbuff);
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                }
            } else { /*拉伸缩放*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-jpg1", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);
                
                scale_buffer = (uint8_t *)heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
                assert(scale_buffer);

                if(s_width < s_height) {
                    ESP_LOGE("picture", "portrait screen s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,width_,height_);
                    s_width = width_;
                    s_height = height_;
                } else {
                    ESP_LOGE("picture", "landscape s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,height_,width_);
                    s_width = height_;
                    s_height = width_;
                }
                dither_.ImgDecode_JPGBufferFree(decimgbuff);

                if (dither_.ImgDecode_DitherRgb888ToOriginalBuffer(scale_buffer, s_width,s_height) == ESP_OK) {
                    EPD_ImgBufferToDispBuffer(scale_buffer, s_width,s_height);
                    free(scale_buffer);
                    scale_buffer = NULL;
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(scale_buffer);
                    scale_buffer = NULL;
                }
            }
        } else { /*解码失败*/
            ESP_LOGE(TAG, "jpg dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_JPGBufferFree(decimgbuff);
            }
        }
    } else if(strstr(path, ".png") || strstr(path, ".PNG")) {
        if(dither_.ImgDecode_TFOnePNGPicture(path,&decimgbuff,&s_width,&s_height) == ESP_OK) {
            if((s_width > 1600 || s_height > 1200) && (s_width > 1200 || s_height > 1600)) {
                dither_.ImgDecode_PNGBufferFree(decimgbuff);
                free(scale_buffer);
                scale_buffer = NULL;

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-end", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

                // PSRAM recycling prevents other programs from disrupting continuity
                if(largest_block < 23 * 512 * 1024){
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
                } else {
                    
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
                }
                assert(BmpSrcBuffer);

                return;
            }
            if((1600 == s_width && 1200 == s_height) || (1200 == s_width && 1600 == s_height)) { /*正常抖动显示*/

                free(scale_buffer);
                scale_buffer = NULL;

                if (dither_.ImgDecode_DitherRgb888ToOriginalBuffer(decimgbuff, s_width,s_height) == ESP_OK) {
                    EPD_ImgBufferToDispBuffer(decimgbuff, s_width,s_height);
                    dither_.ImgDecode_PNGBufferFree(decimgbuff);
                } else {
                    dither_.ImgDecode_PNGBufferFree(decimgbuff);
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                }
            } else {   /*拉伸缩放*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-png1", "Free=%dKB; PSRAM largest free block: %d KB", freel/1024, largest_block / 1024);

                if(s_width < s_height) {
                    ESP_LOGE("picture", "portrait screen s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,width_,height_);
                    s_width = width_;
                    s_height = height_;
                } else {
                    ESP_LOGE("picture", "landscape s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,height_,width_);
                    s_width = height_;
                    s_height = width_;
                }
                dither_.ImgDecode_PNGBufferFree(decimgbuff);

                if (dither_.ImgDecode_DitherRgb888ToOriginalBuffer(scale_buffer, s_width,s_height) == ESP_OK) {
                    EPD_ImgBufferToDispBuffer(scale_buffer, s_width,s_height);
                    free(scale_buffer);
                    scale_buffer = NULL;
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(scale_buffer);
                    scale_buffer = NULL;
                }
            }
        } else {      /*解码失败*/
            ESP_LOGE(TAG, "PNG dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_PNGBufferFree(decimgbuff);
            }
        }
    } else if(strstr(path, ".bmp") || strstr(path, ".BMP")) {
        if(dither_.ImgDecodebmp_TFOneBMPPicture(path,&decimgbuff,&s_width,&s_height) == ESP_OK) {
            if((s_width > 1600 || s_height > 1200) && (s_width > 1200 || s_height > 1600)) {
                ESP_LOGE("width、height", "out of scope");
                dither_.ImgDecode_BMPBufferFree(decimgbuff);
                free(scale_buffer);
                scale_buffer = NULL;

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-end", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

                // PSRAM recycling prevents other programs from disrupting continuity
                if(largest_block < 23 * 512 * 1024){
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
                } else {
                    BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
                }
                assert(BmpSrcBuffer);

                return;
            }
            if((1600 == s_width && 1200 == s_height) || (1200 == s_width && 1600 == s_height)) { /*正常抖动显示*/
                free(scale_buffer);
                scale_buffer = NULL;

                if (dither_.ImgDecode_DitherRgb888ToOriginalBuffer(decimgbuff, s_width,s_height) == ESP_OK) {
                    EPD_ImgBufferToDispBuffer(decimgbuff, s_width,s_height);
                    dither_.ImgDecode_BMPBufferFree(decimgbuff);
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    dither_.ImgDecode_BMPBufferFree(decimgbuff);
                }

            } else {   /*拉伸缩放*/

                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-bmp1", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

                if(s_width < s_height) {
                    ESP_LOGE("picture", "portrait screen s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,width_,height_);
                    s_width = width_;
                    s_height = height_;
                } else {
                    ESP_LOGE("picture", "landscape s_width=%ld, s_height=%ld", s_width ,s_height);
                    dither_.ImgDecode_ScaleRgb888Nearest(decimgbuff,s_width,s_height,scale_buffer,height_,width_);
                    s_width = height_;
                    s_height = width_;
                }
                dither_.ImgDecode_BMPBufferFree(decimgbuff);

                if (dither_.ImgDecode_DitherRgb888ToOriginalBuffer(scale_buffer, s_width,s_height) == ESP_OK) {
                    EPD_ImgBufferToDispBuffer(scale_buffer, s_width,s_height);
                    free(scale_buffer);
                    scale_buffer = NULL;
                } else {
                    ESP_LOGE(TAG, "bmp to sdcard fill");
                    free(scale_buffer);
                    scale_buffer = NULL;
                }
            }
        } else {
            ESP_LOGE(TAG, "BMP dec fill");
            if (decimgbuff != NULL) {
                dither_.ImgDecode_BMPBufferFree(decimgbuff);
            }
        }
    }

    freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("MEM-end", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

    // PSRAM recycling prevents other programs from disrupting continuity
    if(largest_block < 23 * 512 * 1024){
        BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(largest_block, MALLOC_CAP_SPIRAM); //11.5M
    } else {
        
        BmpSrcBuffer              = (uint8_t *)heap_caps_malloc(23 * 512 * 1024, MALLOC_CAP_SPIRAM); //11.5M
    }
    assert(BmpSrcBuffer);
}

void ePaperPort::EPD_DrawStringCN(uint16_t Xstart, uint16_t Ystart, const char * pString, cFONT* font,uint16_t Color_Foreground, uint16_t Color_Background) {
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j,Num;
    uint8_t FONT_BACKGROUND = 0xff;
    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        if(*p_text <= 0xE0) {  //ASCII < 126
            for(Num = 0; Num < font->size; Num++) {
                if(*p_text== font->table[Num].index[0]) {
                    const char* ptr = &font->table[Num].matrix[0];
                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                } else {
                                    EPD_SetPixel(x + i, y + j, Color_Background);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 1;
            /* Decrement the column position by 16 */
            x += font->ASCII_Width;
        } else {        //Chinese
            for(Num = 0; Num < font->size; Num++) {
                if((*p_text== font->table[Num].index[0]) && (*(p_text+1) == font->table[Num].index[1])  && (*(p_text+2) == font->table[Num].index[2])) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                } else {
                                    EPD_SetPixel(x + i, y + j, Color_Background);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 3;
            /* Decrement the column position by 16 */
            x += font->Width;
        }
    }
}

uint8_t ePaperPort::EPD_GetPixel4(const uint8_t* buf, int width, int x, int y) {
    int index = y * (width >> 1) + (x >> 1);
    uint8_t byte = buf[index];
    return (x & 1) ? (byte & 0x0F) : (byte >> 4);
}

void ePaperPort::EPD_SetPixel4(uint8_t* buf, int width, int x, int y, uint8_t px) {
    int index = y * (width >> 1) + (x >> 1);
    uint8_t old = buf[index];
    if (x & 1)
        buf[index] = (old & 0xF0) | (px & 0x0F);
    else
        buf[index] = (old & 0x0F) | (px << 4);
}

void ePaperPort::EPD_PixelRotate() {
    if(Rotation == 3) {
        EPD_Rotate90CCW_Fast(DispBuffer,RotationBuffer,1600,1200);
    } else if(Rotation == 1) {
        EPD_Rotate90CW_Fast(DispBuffer,RotationBuffer,1600,1200);
    } else if(Rotation == 2) {
        EPD_Rotate180_Fast(DispBuffer,RotationBuffer,1200,1600);
    } else {
        memcpy(RotationBuffer, DispBuffer, DisplayLen);
    }
}

void ePaperPort::EPD_Rotate180_Fast(const uint8_t* src, uint8_t* dst, int width, int height)
{
    const int bytesPerRow = width >> 1;
    const int totalRows   = height;    
    for (int y = 0; y < totalRows; y++) {
        const uint8_t* srcRow = src + y * bytesPerRow;
        uint8_t* dstRow = dst + (totalRows - 1 - y) * bytesPerRow;
        for (int x = 0; x < bytesPerRow; x++) {
            uint8_t b = srcRow[x];
            b = (b << 4) | (b >> 4);
            dstRow[bytesPerRow - 1 - x] = b;
        }
    }
}

// src: 原始图像缓存（不含flag），dst: 目标缓存（不含flag），w/h为原始宽高
void ePaperPort::EPD_Rotate90CCW_Fast(const uint8_t* src, uint8_t* dst, int width, int height)
{
    const int srcBytesPerRow = width >> 1;
    for (int y = 0; y < height; y++) {
        const uint8_t* srcRow = src + y * srcBytesPerRow;
        for (int x = 0; x < width; x += 2) {

            uint8_t b = srcRow[x >> 1];
            uint8_t p0 = b >> 4;
            uint8_t p1 = b & 0x0F;
            int ny0 = width - 1 - x;
            int nx0 = y;
            int ny1 = width - 2 - x;
            int nx1 = y;
            EPD_SetPixel4(dst, height, nx0, ny0, p0);
            EPD_SetPixel4(dst, height, nx1, ny1, p1);
        }
    }
}

void ePaperPort::EPD_Rotate90CW_Fast(const uint8_t* src, uint8_t* dst, int width, int height)
{
    const int srcBytesPerRow = width >> 1;
    for (int y = 0; y < height; y++) {
        const uint8_t* srcRow = src + y * srcBytesPerRow;
        for (int x = 0; x < width; x += 2) {

            uint8_t b = srcRow[x >> 1];
            uint8_t p0 = b >> 4;  
            uint8_t p1 = b & 0x0F;
            int ny0 = x;
            int nx0 = height - 1 - y;
            int ny1 = x + 1;
            int nx1 = height - 1 - y;
            EPD_SetPixel4(dst, height, nx0, ny0, p0);
            EPD_SetPixel4(dst, height, nx1, ny1, p1);
        }
    }
}

void ePaperPort::EPD_DrawChar(uint16_t Xpoint, uint16_t Ypoint, const char Acsii_Char,sFONT* Font, uint16_t Color_Foreground, uint16_t Color_Background) {
    uint16_t Page, Column;

    if (Xpoint > width_ || Ypoint > height_) {
        ESP_LOGE(TAG,"Paint_DrawChar Input exceeds the normal display range");
        return;
    }

    uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    const unsigned char *ptr = &Font->table[Char_Offset];

    for (Page = 0; Page < Font->Height; Page ++ ) {
        for (Column = 0; Column < Font->Width; Column ++ ) {

            //To determine whether the font background color and screen background color is consistent
            if (0XFF == Color_Background) { //this process is to speed up the scan
                if (*ptr & (0x80 >> (Column % 8)))
                    EPD_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            } else {
                if (*ptr & (0x80 >> (Column % 8))) {
                    EPD_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                } else {
                    EPD_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
                    // Paint_DrawPoint(Xpoint + Column, Ypoint + Page, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                }
            }
            //One pixel is 8 bits
            if (Column % 8 == 7)
                ptr++;
        }// Write a line
        if (Font->Width % 8 != 0)
            ptr++;
    }// Write all
}

void ePaperPort::EPD_DrawStringEN(uint16_t Xstart, uint16_t Ystart, const char * pString,sFONT* Font, uint16_t Color_Foreground, uint16_t Color_Background) {
    uint16_t Xpoint = Xstart;
    uint16_t Ypoint = Ystart;

    if (Xstart > width_ || Ystart > height_) {
        ESP_LOGE(TAG,"Paint_DrawString_EN Input exceeds the normal display range");
        return;
    }

    while (* pString != '\0') {
        //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
        if ((Xpoint + Font->Width ) > width_ ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > height_ ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        EPD_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Background, Color_Foreground);

        //The next character of the address
        pString ++;

        //The next word of the abscissa increases the font of the broadband
        Xpoint += Font->Width;
    }
}


void ePaperPort::EPD_BmpSrcBuffer_flee(){
    if(BmpSrcBuffer != NULL){
        printf("EPD_BmpSrcBuffer_flee\r\n");
        free(BmpSrcBuffer);
        BmpSrcBuffer = NULL;
    }
}

uint8_t* ePaperPort::EPD_DispBuffer(){
    return DispBuffer;
}

uint8_t* ePaperPort::EPD_RotationBuffer(){
    return RotationBuffer;
}

uint8_t* ePaperPort::EPD_BmpSrcBuffer(){
    return BmpSrcBuffer;
}


