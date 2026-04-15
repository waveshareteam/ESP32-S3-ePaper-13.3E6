#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include "user_app.h"
#include "button_bsp.h"
#include "ai_app.h"
#include "list.h"

static int sdcard_Basic_count = 0; 
static uint8_t           Basic_sleep_arg = 0; // Parameters for low-power tasks
static SemaphoreHandle_t sleep_Semp;          // Binary call low-power task 
static uint8_t           wakeup_basic_flag = 0;
static list_t* ListHost;
BaseAIModel *model = NULL;

static void ePaper_user_Task(void *arg) {
    uint8_t *wakeup_arg = (uint8_t *) arg;
    ePaperDisplay.EPD_DispClear(ColorWhite);
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(UserButtonGroups, set_bit_button(1), pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 1)) {
            if (*wakeup_arg == 0) {
                if (pdTRUE == xSemaphoreTake(epaper_gui_semapHandle, 2000)) {                       
                    list_node_t *sdcard_node = list_at(ListHost, sdcard_Basic_count); 
                    if (sdcard_node == NULL) {
                        sdcard_Basic_count = 0;
                        sdcard_node        = list_at(ListHost, sdcard_Basic_count);
                    }
                    ESP_LOGW("node", "%ld", sdcard_Basic_count);
                    sdcard_Basic_count++;

                    int write_ret = model->BaseAIModel_SdcardWriteAIModelImgIndex(sdcard_Basic_count);
                    if (write_ret < 0) {  // Failure returns -1, success returns >=0
                        ESP_LOGE("img_index", "Write failure, ret=%d", write_ret);
                    }
                    
                    if (sdcard_node != NULL) {
                        CustomSDPortNode_t *sdcard_Name_node = (CustomSDPortNode_t *) sdcard_node->val;
                        
                        ESP_LOGW("name", "%s", sdcard_Name_node->sdcard_name);
                        
                        EPD_PWR_ON_Task();
                        // // encode and decode and store to the TF card
                        // ePaperDisplay.EPD_SDcardScaleIMGShakingColor(sdcard_Name_node->sdcard_name,0,0);

                        // Refresh directly after decoding without saving
                        ePaperDisplay.EPD_SDcardScaleIMGShakingColor_NotSave(sdcard_Name_node->sdcard_name);

                        ePaperDisplay.EPD_Init();
                        vTaskDelay(pdMS_TO_TICKS(100));
                        ePaperDisplay.EPD_Display();
                        vTaskDelay(pdMS_TO_TICKS(100));
                        ePaperDisplay.EPD_Sleep();
                        vTaskDelay(pdMS_TO_TICKS(100));

                        xSemaphoreGive(epaper_gui_semapHandle); 

                    }
                }
            }
        }
    }
}

static void ePaper_user_Task_BAT(void) {               
    list_node_t *sdcard_node = list_at(ListHost, sdcard_Basic_count); 
    if (sdcard_node == NULL) {
        sdcard_Basic_count = 0;
        sdcard_node        = list_at(ListHost, sdcard_Basic_count);
    }
    ESP_LOGW("node", "%ld", sdcard_Basic_count);
    sdcard_Basic_count++;

    int write_ret = model->BaseAIModel_SdcardWriteAIModelImgIndex(sdcard_Basic_count);
    if (write_ret < 0) {
        ESP_LOGE("img_index", "Write failure, ret=%d", write_ret);
    }
    if (sdcard_node != NULL) 
    {
        CustomSDPortNode_t *sdcard_Name_node = (CustomSDPortNode_t *) sdcard_node->val;
        
        ESP_LOGW("name", "%s", sdcard_Name_node->sdcard_name);
        
        EPD_PWR_ON_Task();

        ePaperDisplay.EPD_SDcardScaleIMGShakingColor_NotSave(sdcard_Name_node->sdcard_name);
        ePaperDisplay.EPD_Init();
        vTaskDelay(pdMS_TO_TICKS(100));
        ePaperDisplay.EPD_Display();
        vTaskDelay(pdMS_TO_TICKS(100));
        ePaperDisplay.EPD_Sleep();
        vTaskDelay(pdMS_TO_TICKS(100));
        
        EPD_PWR_OFF_Task();
    } else {
        ESP_LOGE("SD", "The directory was not obtained");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    EventBits_t even = xEventGroupWaitBits(power_groups, set_bit_button(0), pdFALSE, pdFALSE, portMAX_DELAY);
    if (get_bit_data(even, 0)){
        xTaskCreate(ePaper_user_Task, "e-Paper_user_Task", 6 * 1024, &wakeup_basic_flag, 3, NULL);
    }
}


void User_Basic_mode_app_init(void) {
    ListHost = SDPort->SDPort_GetListHost();
    sleep_Semp  = xSemaphoreCreateBinary();
    model = new BaseAIModel(SDPort, decdither);
    BaseAIModelConfig_t *AIModelConfig = NULL;
    AIModelConfig = model->BaseAIModel_SdcardReadAIModelConfig();
    SDPort->SDPort_ScanListDir("/sdcard/img"); 
    ESP_LOGW("IMG","Values:%d",SDPort->Get_Sdcard_ImgValue());  

    sdcard_Basic_count = model->BaseAIModel_SdcardReadAIModelImgIndex();
    ESP_LOGW("sdcard_Basic_count","sdcard_Basic_count:%d",sdcard_Basic_count);  

    xTaskCreate(ePaper_user_Task, "e-Paper_user_Task", 6 * 1024, &wakeup_basic_flag, 3, NULL);

}

