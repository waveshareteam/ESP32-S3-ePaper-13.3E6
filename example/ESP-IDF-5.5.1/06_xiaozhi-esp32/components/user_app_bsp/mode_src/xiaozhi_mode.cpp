#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <driver/rtc_io.h>
#include "user_app.h"
#include "ai_app.h"
#include "application.h"
#include "client_app.h"
#include "weather_app.h"
#include "button_bsp.h"
#include "list.h"
#include "i2c_equipment.h"
#include <esp_sleep.h>

//#include "esp_mac.h"

static const char *TAG = "xiaozhi_mode";
BaseAIModel *AiModel = NULL;
WeatherPort WeaPort;
WeatherData_t *WeatherData = NULL;          
static list_t *ListHost = NULL;
uint8_t *bmp_buf = NULL;

char THData[40];
char BAT_level[40];
int                sdcard_bmp_Quantity = 0; // The number of images in the sdcard directory  // Used in Xiaozhi main code
int                sdcard_doc_count    = 0; // The index of the image  // Used in Xiaozhi main code
int                is_ai_img           = 1; // If the current process is refreshing, then the AI-generated images cannot be generated
EventGroupHandle_t ai_IMG_Group;            // Task group for ai_IMG

char   *str_ai_chat_buff = NULL; // This is a text-to-image conversion. The default text length is 1024.
list_t *sdcard_score     = NULL; // The high-score list requires memory allocation and deallocation

char sleep_buff[18]; 

SemaphoreHandle_t ai_img_while_semap; 

EventGroupHandle_t ai_IMG_LoopGroup;  // AI image loop event group
int img_loopTimer = 1 * 60;    // Default 1 minute
int img_loopCount = 0;                // Loop count
int img_CurrentTimer = 0;             // Current timing
bool ai_IMG_LoopTask_Flag = false;    

/**
 * 0x00 leisure
 * 0x01 Display image on the SD card
 * 0x02 AI image generated
 */
char ePaper_Refresh_Flag = 0x00;       


void xiaozhi_init_received(const char *arg1) 
{
    static uint8_t Oneime = 0;
    if (Oneime)
        return;
    if (strstr(arg1, "版本") != NULL) {
        Oneime          = 1; 
        std::string wake_word = "你好小智";
        Application::GetInstance().WakeWordInvoke(wake_word);
        xEventGroupSetBits(epaper_groups, set_bit_button(0));
    }
}

void xiaozhi_application_received(const char *str) {
    static bool is_led_flag = false;
    strcpy(sleep_buff, str);
    if (is_led_flag) {
        if (strstr(sleep_buff, "idle") != NULL) {
            // gpio_set_level((gpio_num_t) 45, 1);
            // is_led_flag = false;
        }
    } else {
        if ((strstr(sleep_buff, "listening") != NULL) || (strstr(sleep_buff, "speaking") != NULL)) {
            // gpio_set_level((gpio_num_t) 45, 0);
            // is_led_flag = true;
        }
    }
}

void xiaozhi_ai_Message(const char *arg1, const char *arg2) //ai chat
{
    if (arg1 == NULL || arg2 == NULL) {
        return;
    }

    if (str_ai_chat_buff == NULL) {
        ESP_LOGE("chat", "str_ai_chat_buff is NULL!!");
        return;
    }

    if (!strcmp(arg1, "user")) {
        strcpy(str_ai_chat_buff, arg2);
    }
}

static void gui_user_Task(void *arg) {
    int *sdcard_doc = (int *) arg;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(epaper_groups, set_bit_all, pdTRUE, pdFALSE, portMAX_DELAY); 
        if (pdTRUE == xSemaphoreTake(epaper_gui_semapHandle, 2000)) {         
            ePaper_Refresh_Flag = 0x01;
            if (get_bit_button(even, 0)) {  // Show the weather
                // When starting up, reserve the interface
            } else if (get_bit_button(even, 1)) {   // Switch the pictures in the local or SD card
                // xEventGroupClearBits(ai_IMG_LoopGroup, 0x01);  
                *sdcard_doc -= 1;
                list_node_t *sdcard_node = list_at(ListHost, *sdcard_doc); 
                if (sdcard_node != NULL) {
                    CustomSDPortNode_t *sdcard_Name_node = (CustomSDPortNode_t *) sdcard_node->val;
                    SDPort->SDPort_SetCurrentlyNode(sdcard_node);
                    ESP_LOGW(TAG,"voice_Sort:%d,list_Sort:%d,path:%s",(*sdcard_doc+1),*sdcard_doc,sdcard_Name_node->sdcard_name);

                    EPD_PWR_ON_Task();
                    ePaperDisplay.EPD_SDcardScaleIMGShakingColor_NotSave(sdcard_Name_node->sdcard_name);

                    ePaperDisplay.EPD_Init();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ePaperDisplay.EPD_Display();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ePaperDisplay.EPD_Sleep();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    
                    EPD_PWR_OFF_Task();

                    img_CurrentTimer = 0;

                }
            } else if (get_bit_button(even, 2)) {   // Voice display picture
                size_t total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
                size_t freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                ESP_LOGI("PSRAM", "S3-N32R16 PSRAM: Total=%dKB (%.2fMB), Free=%dKB", total/1024, (float)total/(1024*1024), freel/1024);
                   
                EPD_PWR_ON_Task();

                ePaperDisplay.EPD_SDcardScaleIMGShakingColor_NotSave(AiModel->Get_AiTFImgName());

                ePaperDisplay.EPD_Init();
                vTaskDelay(pdMS_TO_TICKS(100));
                ePaperDisplay.EPD_Display();
                vTaskDelay(pdMS_TO_TICKS(100));
                ePaperDisplay.EPD_Sleep();
                vTaskDelay(pdMS_TO_TICKS(100));
                
                EPD_PWR_OFF_Task();

                // Reset Timers
                img_CurrentTimer = 0;
                
                freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
                size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                ESP_LOGI("MEM-end", "Free=%dKB; PSRAM largest free block: %d KB",  freel/1024, largest_block / 1024);

            } else if (get_bit_button(even, 3)) {   // Refresh the picture at regular intervals
                img_loopCount--;                  
                list_node_t *node = list_at(ListHost, img_loopCount);
                if (node != NULL) {
                    CustomSDPortNode_t *sdcard_Name_node_ai = (CustomSDPortNode_t *) node->val;
                    SDPort->SDPort_SetCurrentlyNode(node);
                    ESP_LOGW(TAG,"loop_Sort:%d,list_Sort:%d,path:%s",(img_loopCount+1),img_loopCount,sdcard_Name_node_ai->sdcard_name);

                    EPD_PWR_ON_Task();

                    ePaperDisplay.EPD_SDcardScaleIMGShakingColor_NotSave(sdcard_Name_node_ai->sdcard_name);

                    ePaperDisplay.EPD_Init();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ePaperDisplay.EPD_Display();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    ePaperDisplay.EPD_Sleep();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    
                    EPD_PWR_OFF_Task();

                    // Reset Timers
                    img_CurrentTimer = 0;
                }
                if(img_loopCount == 0) {
                    img_loopCount = sdcard_bmp_Quantity;
                }
            }
            xSemaphoreGive(epaper_gui_semapHandle); 
            ePaper_Refresh_Flag = 0x00;                  
        }
    }
}

    static void ai_IMG_Task(void *arg) {
    char *chatStr = (char *) arg;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(ai_IMG_Group, (0x01) | (0x02) | (0x08), pdTRUE, pdFALSE, portMAX_DELAY);
        if (get_bit_button(even, 0)) {
            ESP_LOGW("chat", "%s", chatStr);
            // xEventGroupClearBits(ai_IMG_LoopGroup, 0x01);
            ePaper_Refresh_Flag = 0x02;
            AiModel->BaseAIModel_SetChat(chatStr);         
            char *str = AiModel->BaseAIModel_GetImgName();
            if (str != NULL) {
                ESP_LOGW("ai_IMG_Task", "Generated image path: %s", str); 
                SDPort->SDPort_ScanListDir("/sdcard/img");       // Place the image data under the linked list
                sdcard_bmp_Quantity = SDPort->SDPort_GetScanListValue();    // Traverse the linked list to count the number of images
                xEventGroupSetBits(epaper_groups, set_bit_button(2));                
            }
            ePaper_Refresh_Flag = 0x00;
        } else if (get_bit_button(even, 1)) {
            sdcard_bmp_Quantity = SDPort->SDPort_GetScanListValue(); 
            xSemaphoreGive(ai_img_while_semap);    
        } else if (get_bit_button(even, 3)) {              
            auto &app = Application::GetInstance();
            if (strstr(sleep_buff, "idle") != NULL) {

            } else if (strstr(sleep_buff, "listening") != NULL) {
                app.ToggleChatState();
            } else if (strstr(sleep_buff, "speaking") != NULL) {
                app.ToggleChatState();
                vTaskDelay(pdMS_TO_TICKS(500));
                app.ToggleChatState();
            }
        }
    }
    vTaskDelete(NULL);
}

void key_wakeUp_user_Task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(UserButtonGroups, (0x01), pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (even & 0x01) {
            if (strstr(sleep_buff, "idle") != NULL) {
                // gpio_set_level((gpio_num_t) 45, 0);
                std::string wake_word = "你好小智";
                Application::GetInstance().WakeWordInvoke(wake_word);
            }
        }
    }
}

void pwr_sleep_user_Task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(UserButtonGroups, (0x01), pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (even & 0x01) {
            xEventGroupSetBits(ai_IMG_Group, 0x08);
            gpio_set_level((gpio_num_t) 45, 1); 
        }
    }
}

char* Get_BatteryLevel(void) {
    uint8_t level = Get_BAT_level();
    // ESP_LOGE("电量：","%d", level);
    snprintf(BAT_level,sizeof(BAT_level),"电量:%d%%",level);
    return BAT_level;
}


void ai_IMG_LoopTask(void *arg) {

    for (;;) {
        if(ai_IMG_LoopTask_Flag && (ePaper_Refresh_Flag == 0x00)){
            img_CurrentTimer++;
            // ESP_LOGE("ai_IMG_LoopTask", "%d", img_CurrentTimer);
            if(img_CurrentTimer > img_loopTimer){
                xEventGroupSetBits(epaper_groups, set_bit_button(3));
            }
        } else if(!ai_IMG_LoopTask_Flag){
            img_CurrentTimer = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void User_xiaozhi_app_init(void)                        // Initialization in the Xiaozhi mode
{

    ListHost = SDPort->SDPort_GetListHost();
    AiModel = new BaseAIModel(SDPort,decdither,1200,1600);
    BaseAIModelConfig_t* AIconfig = AiModel->BaseAIModel_SdcardReadAIModelConfig();
    if (AIconfig != NULL) {                             //Obtain key, url, model
        ESP_LOGI("ai_model", "model:%s,key:%s,url:%s", AIconfig->model,AIconfig->key,AIconfig->url);
    } else {
        ESP_LOGE("ai_model", "error");
        return;
    }
    AiModel->BaseAIModel_AIModelInit(AIconfig->model,AIconfig->url,AIconfig->key);
    gpio_set_level((gpio_num_t) 45, 0);
    ai_img_while_semap = xSemaphoreCreateBinary();

    size_t total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t freel = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGE("PSRAM", "S3-N32R16 PSRAM: Total=%dKB (%.2fMB), Free=%dKB", total/1024, (float)total/(1024*1024), freel/1024);

    str_ai_chat_buff   = (char *) heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    ai_IMG_Group       = xEventGroupCreate();
    ai_IMG_LoopGroup       = xEventGroupCreate();
    SDPort->SDPort_ScanListDir("/sdcard/img");       // Place the image data under the linked list
    sdcard_bmp_Quantity = SDPort->SDPort_GetScanListValue();    // Traverse the linked list to count the number of images
    img_loopCount = sdcard_bmp_Quantity;

    bmp_buf = ePaperDisplay.EPD_BmpSrcBuffer();

    xTaskCreate(gui_user_Task, "gui_user_Task", 6 * 1024, &sdcard_doc_count, 2, NULL);
    xTaskCreate(ai_IMG_Task, "ai_IMG_Task", 6 * 1024, str_ai_chat_buff, 2, NULL);
    xTaskCreate(ai_IMG_LoopTask, "ai_IMG_LoopTask", 4 * 1024, NULL, 2, NULL);
    xTaskCreate(key_wakeUp_user_Task, "key_wakeUp_user_Task", 4 * 1024, NULL, 3, NULL); 
    xTaskCreate(pwr_sleep_user_Task, "pwr_sleep_user_Task", 4 * 1024, NULL, 3, NULL); 
}
