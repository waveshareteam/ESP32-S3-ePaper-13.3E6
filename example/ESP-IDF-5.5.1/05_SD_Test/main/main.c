#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"
#include "epaper_port.h"
#include "ImageData.h"
#include "sdcard_bsp.h"
#include "esp_log.h"

// Define the data cache area of the e-ink screen
uint8_t *Image_Mono = NULL;
// Log tag
static const char *TAG = "main";

size_t buffer_size =  EPD_WIDTH * EPD_HEIGHT / 2;

void app_main(void)
{
    _sdcard_init();
    if(card_host == NULL) {
        ESP_LOGE(TAG,"SD card init failed");
        return;
    }else{
        ESP_LOGI(TAG,"SD card init success");
    }

    //traverse the BMP file and printf
    // sdcard_queuehandle = xQueueCreate(20, sizeof(char)*300);
    // scan_files("/sdcard/bmp");

    ESP_LOGI(TAG,"1.e-Paper Init and Clear...");
    EPD_Port_Init();
    EPD_Init();
    EPD_Clear(EPD_WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    if((Image_Mono = (uint8_t *)heap_caps_malloc(buffer_size,MALLOC_CAP_SPIRAM)) == NULL){
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        return;
    } 
    Paint_NewImage(Image_Mono, EPD_WIDTH, EPD_HEIGHT, 0, EPD_WHITE);
    Paint_SetScale(6);

#if 0
    EPD_Display(Image6color);
    vTaskDelay(pdMS_TO_TICKS(3000));
#endif

#if 1
    ESP_LOGI(TAG,"show sdcard BMP-----------------");
    Paint_Clear(WHITE);
    GUI_ReadBmp_RGB_6Color("/sdcard/bmp/13in3E.bmp", 0, 0);
    EPD_Display(Image_Mono);
    vTaskDelay(pdMS_TO_TICKS(3000));
#endif

#if 1
    //Call the font library in the SD card
    Paint_NewImage(Image_Mono, EPD_WIDTH, EPD_HEIGHT, 90, EPD_WHITE);
    Paint_SetScale(6);
    Paint_SelectImage(Image_Mono);
    Paint_Clear(EPD_WHITE);

    Paint_DrawPoint(10, 80, EPD_RED, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 90, EPD_BLUE, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 100, EPD_GREEN, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawLine(20, 70, 70, 120, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 70, 20, 120, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawRectangle(20, 70, 70, 120, EPD_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(80, 70, 130, 120, EPD_BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawCircle(45, 95, 20, EPD_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(105, 95, 20, EPD_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawLine(85, 95, 125, 95, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(105, 75, 105, 115, EPD_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawString_CN(10, 130, "你好abc", &Font16CN, EPD_BLACK, EPD_WHITE);
    Paint_DrawString_CN(10, 170, "微雪电子", &Font16CN, EPD_WHITE, EPD_BLACK);
    Paint_DrawNum(10, 10, 123456789, &Font12, EPD_BLACK, EPD_WHITE);
    Paint_DrawNum(10, 40, 987654321, &Font12, EPD_WHITE, EPD_BLACK);
    Paint_DrawString_EN(145, 0, "Waveshare", &Font16, EPD_BLACK, EPD_WHITE);
    Paint_DrawString_EN(145, 35, "Waveshare", &Font16, EPD_GREEN, EPD_WHITE);
    Paint_DrawString_EN(145, 70, "Waveshare", &Font16, EPD_BLUE, EPD_WHITE);
    Paint_DrawString_EN(145, 105, "Waveshare", &Font16, EPD_RED, EPD_WHITE);
    Paint_DrawString_EN(145, 140, "Waveshare", &Font16, EPD_YELLOW, EPD_WHITE);

    EPD_Display(Image_Mono);
    vTaskDelay(pdMS_TO_TICKS(3000));
#endif

    ESP_LOGI(TAG,"clear and go to sleep");

    EPD_Clear(EPD_WHITE);
    vTaskDelay(pdMS_TO_TICKS(2000));

    EPD_Sleep();
    ESP_LOGI(TAG,"close 5V, Module enters 0 power consumption ...");
    vTaskDelay(pdMS_TO_TICKS(2000));

}