#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include "display_bsp.h"
#include "server_app.h"
#include "button_bsp.h"
#include "user_app.h"
#include "traverse_nvs.h"


TraverseNvs *nvs_viewer = NULL;
static const char *TAG = "NetWorkMode";

uint8_t NetWorkMode = 0;
uint8_t *img_buf = NULL;
uint8_t *epd_Image = NULL;

// 0 0°/180°  ,   1 90°/270°
uint8_t rotary_flag = false;
// 0 AP  ,   1 STA
uint8_t AP_OR_STA_flag = false;

uint8_t Get_nvsNetworkMode(void) {
    esp_err_t ret;
    nvs_handle_t my_handle;
    ret = nvs_open("PhotoPainter", NVS_READWRITE, &my_handle);
    ESP_ERROR_CHECK(ret);
    uint8_t netMode = 0;
    ret                = nvs_get_u8(my_handle, "NetworkMode", &netMode);
    ESP_ERROR_CHECK(ret);
    nvs_close(my_handle); 
    return netMode;
}

void Set_nvsNetworkMode(uint8_t mode) {
    esp_err_t ret;
    nvs_handle_t my_handle;
    ret = nvs_open("PhotoPainter", NVS_READWRITE, &my_handle);
    ESP_ERROR_CHECK(ret);
    uint8_t netMode = 0;
    ret                = nvs_get_u8(my_handle, "NetworkMode", &netMode);
    ESP_ERROR_CHECK(ret);
    if(netMode != mode) {
        ret = nvs_set_u8(my_handle, "NetworkMode", mode);
        ESP_ERROR_CHECK(ret);
        nvs_commit(my_handle);
    }
    nvs_close(my_handle); 
}

uint8_t Get_CurrentlyNetworkMode(void) {
    return NetWorkMode;
}

static void Network_user_Task(void *arg) {
    ePaperDisplay.EPD_DispClear(ColorWhite);
    ePaperDisplay.EPD_BmpSrcBuffer_flee();
    
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(ServerPortGroups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        
        if (get_bit_button(even, 0)) { 
        } else if (get_bit_button(even, 1)) {  
        } else if (get_bit_button(even, 2)) {  
            if (pdTRUE == xSemaphoreTake(epaper_gui_semapHandle, 2000)) {
                
                if(rotary_flag){
                    printf("EPD_Rotate90CCW_Fast, rotary_flag = %d\r\n", rotary_flag);
                    ePaperDisplay.EPD_Rotate90CW_Fast(img_buf, epd_Image, EPD_HEIGHT, EPD_WIDTH);
                } else {
                    printf("memcpy, rotary_flag = %d\r\n", rotary_flag);
                    memcpy(epd_Image, img_buf, IMG_PIXELS);
                }

                EPD_PWR_ON_Task();
                vTaskDelay(pdMS_TO_TICKS(100));

                // Initialize and refresh the screen
                ePaperDisplay.EPD_Init();
                vTaskDelay(pdMS_TO_TICKS(100));
                ePaperDisplay.EPD_Display();
                vTaskDelay(pdMS_TO_TICKS(100));
                

                xSemaphoreGive(epaper_gui_semapHandle); 

                if(NetWorkMode != AP_OR_STA_flag) {
                    NetWorkMode = AP_OR_STA_flag;
                    Set_nvsNetworkMode(NetWorkMode);
                }
            }
        } 
        
        else if (get_bit_button(even, 4) | get_bit_button(even, 6)) {  
        }
    }
}

static void boot_button_user_Task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(UserButtonGroups, set_bit_button(2), pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 2)) {
            // double-click the BOOT key: Reset to AP mode and restart
            Set_nvsNetworkMode(0);
            esp_restart();
        }
    }
}


void User_Network_mode_app_init(void) {
    img_buf = ePaperDisplay.EPD_RotationBuffer();
    epd_Image = ePaperDisplay.EPD_DispBuffer();

    if((NetWorkMode = Get_nvsNetworkMode())) {
        ESP_LOGW(TAG, "STA mode");
        nvs_viewer = new TraverseNvs();
        wifi_credential_t creden = nvs_viewer->Get_WifiCredentialFromNVS();
        
        if(0 == creden.is_valid) {
            printf("creden.is_valid \r\n");
            xTaskCreate(boot_button_user_Task, "boot_button_user_Task", 6 * 1024, NULL, 3, NULL);
            return;
        }
        
        uint8_t res = ServerPort_NetworkSTAInit(creden); 
        if(0 == res) {
            printf("res \r\n");
            xTaskCreate(boot_button_user_Task, "boot_button_user_Task", 6 * 1024, NULL, 3, NULL);
            return;
        }
        Mdns_init_config();
    } else {
        ESP_LOGW(TAG, "AP mode");
        ServerPort_NetworkAPInit();
    }
    
    ServerPort_init(SDPort);                                                   
    
    xTaskCreate(Network_user_Task, "Network_user_Task", 6 * 1024, NULL, 2, NULL);

}