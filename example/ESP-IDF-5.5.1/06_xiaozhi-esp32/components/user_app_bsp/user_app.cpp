#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <driver/rtc_io.h>
#include "user_app.h"
#include "led_bsp.h"
#include "button_bsp.h"
#include "power_bsp.h"
#include "led_bsp.h"
#include "imgdecode_app.h"

RTC_Pcf85063Port *RTCPort = NULL;
CustomSDPort *SDPort = NULL;
ImgDecodeDither decdither;
ePaperPort ePaperDisplay(decdither,EPD_MOSI_PIN,EPD_SCLK_PIN,EPD_DC_PIN,EPD_CS_M_PIN,EPD_CS_S_PIN,EPD_RST_PIN,EPD_BUSY_PIN,EPD_WIDTH,EPD_HEIGHT,EPD_WIDTH_MAX,EPD_HEIGHT_MAX);
I2cMasterBus I2cBus(ESP32_SCL_NUM,ESP32_SDA_NUM,ESP32_I2C_PORT);

SemaphoreHandle_t  epaper_gui_semapHandle = NULL; // Mutual exclusion lock to prevent repeated refreshing
EventGroupHandle_t epaper_groups;                 // Event group for map refreshing

uint8_t Current_mode = 0;



static void in_mode_user_Task(void *arg) {
    esp_err_t ret;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(UserButtonGroups, set_bit_button(3), pdFALSE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 3)) { 
            nvs_handle_t my_handle;
            ret = nvs_open("PhotoPainter", NVS_READWRITE, &my_handle);
            ESP_ERROR_CHECK(ret);
            uint8_t Mode_value = 0;
            ret                = nvs_get_u8(my_handle, "Mode_Flag", &Mode_value);
            ESP_ERROR_CHECK(ret);
            if (Mode_value == 0x01) { 
                xEventGroupClearBits(UserButtonGroups, set_bit_button(1)); 
                ret = nvs_set_u8(my_handle, "Mode_Flag", 0x00);
                ESP_ERROR_CHECK(ret);
                ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x04);
                ESP_ERROR_CHECK(ret);
                nvs_commit(my_handle);
                nvs_close(my_handle); 
                esp_restart();
            }
        }
    }
}

uint8_t User_Mode_init(void) 
{
    // Power and LED init
    Power_Init();

    epaper_gui_semapHandle = xSemaphoreCreateMutex(); /* Acquire the mutual exclusion lock to prevent re-flashing */                                 
    SDPort = new CustomSDPort("/sdcard");
    uint8_t sdcard_win = SDPort->SDPort_GetSdcardInitOK();              /* SD Card Initialization */
    if (sdcard_win == 0)
        return 0;
    epaper_groups        = xEventGroupCreate();

    Custom_ButtonInit();
    if(Current_mode == 4){
        return 1;
    } else {
        xTaskCreate(in_mode_user_Task, "in_mode_user_Task", 4 * 1024, NULL, 3, NULL);
        return 1;
    }
    
}
