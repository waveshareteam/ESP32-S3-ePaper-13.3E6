#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "application.h"
#include "system_info.h"

#include "user_app.h"
#include "esp_heap_caps.h"

#define TAG "main"

extern "C" void app_main(void) {
    // Initialize the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize NVS flash for WiFi configuration
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    nvs_handle_t my_handle;
    ret = nvs_open("PhotoPainter", NVS_READWRITE, &my_handle);
    ESP_ERROR_CHECK(ret);
    uint8_t read_value = 0;
    ret                = nvs_get_u8(my_handle, "NetworkMode", &read_value);
    if (ret != ESP_OK) {
        ret = nvs_set_u8(my_handle, "NetworkMode", 0x00);
        ESP_ERROR_CHECK(ret);
        nvs_commit(my_handle);  //Submit the revisions
    }
    ret                = nvs_get_u8(my_handle, "PhotPainterMode", &Current_mode);
    if (ret != ESP_OK) {
        ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x03);
        ESP_ERROR_CHECK(ret);
        nvs_commit(my_handle);  //Submit the revisions
        ret = nvs_get_u8(my_handle, "PhotPainterMode", &Current_mode);
    }
    uint8_t Mode_value;
    ret = nvs_get_u8(my_handle, "Mode_Flag", &Mode_value);
    if (ret != ESP_OK) {
        ret = nvs_set_u8(my_handle, "Mode_Flag", 0x01);
        ESP_ERROR_CHECK(ret);
        nvs_commit(my_handle);  //Submit the revisions
        ret = nvs_get_u8(my_handle, "Mode_Flag", &Mode_value);
    }
    nvs_close(my_handle);       //Close handle
    ESP_LOGI("Mode_value", "%d", Mode_value);
    ESP_LOGI("Current mode", "%d", Current_mode);
    /*Button Press Task Creation*/
    if (User_Mode_init() == 0) {
        ESP_LOGE("init", "init Failure");
        return;
    }

    size_t total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ESP_LOGI("PSRAM", "S3-N32R16 PSRAM: Total=%dKB (%.2fMB), Free=%dKB", total/1024, (float)total/(1024*1024), free/1024);
    ESP_LOGI("MEM", "PSRAM largest free block: %d KB", largest_block / 1024);

    if (Current_mode == 0x03) {
        ESP_LOGW("main","Enter xiaozhi mode");
        auto &app = Application::GetInstance();
        app.Start();
    } else if (Current_mode == 0x01) {
        ESP_LOGW("main","Enter Basic mode");
        User_Basic_mode_app_init();
    } else if (Current_mode == 0x02) {
        ESP_LOGW("main","Enter Network mode");
        User_Network_mode_app_init();
    } else if (Current_mode == 0x04) {
        ESP_LOGW("main","Enter Mode Selection");
        Mode_Selection_Init();
    }
}
