#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <driver/rtc_io.h>
#include "user_app.h"
#include "button_bsp.h"
#include "codec_bsp.h"

#include "power_bsp.h"

CodecPort *AudioPort = NULL;

EventGroupHandle_t audio_groups;

// When powered by the battery, the device shuts down after setting the mode for a period of time without any operation
#define Mode_Power_Time  30  // The shutdown Time is Mode_Power_Time * 2 seconds

static void mode_selection_user_Task(void *arg) {
    esp_err_t ret;
    uint8_t   Mode = 0;
    uint32_t   count = 0;
    EventBits_t  even;

    AUDIO_PWR_ON_Task();
    // Wait for the key to be released
    even = xEventGroupWaitBits(UserButtonGroups, set_bit_button(6), pdTRUE, pdFALSE, portMAX_DELAY);
    for (;;) {
        even = xEventGroupWaitBits(UserButtonGroups, set_bit_button(1) | set_bit_button(3) | set_bit_button(6), pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 3)) {
            count = 0;
            if (Mode > 0) {
                nvs_handle_t my_handle;
                ret = nvs_open("PhotoPainter", NVS_READWRITE, &my_handle);
                ESP_ERROR_CHECK(ret);
                vTaskDelay(pdMS_TO_TICKS(2));
                if (Mode == 1) {
                    ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x01);
                    ESP_ERROR_CHECK(ret);
                    vTaskDelay(pdMS_TO_TICKS(2));
                } else if (Mode == 2) {
                    ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x02);
                    ESP_ERROR_CHECK(ret);
                    vTaskDelay(pdMS_TO_TICKS(2));
                } else if (Mode == 3) {
                    ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x03);
                    ESP_ERROR_CHECK(ret);
                    vTaskDelay(pdMS_TO_TICKS(2));
                }
                ret = nvs_set_u8(my_handle, "Mode_Flag", 0x01);
                ESP_ERROR_CHECK(ret);
                vTaskDelay(pdMS_TO_TICKS(2));
                ESP_LOGW("Audio","0x%02x,0x%02x",AudioPort->Codec_GetCodecReg("es8311",0x00),AudioPort->Codec_GetCodecReg("es7210",0x00));
                uint8_t regs = AudioPort->Codec_GetCodecReg("es8311",0xfa);
                ESP_LOGW("es8311 reg","0x%02x",regs);
                AudioPort->Codec_SetCodecReg("es8311", 0xfa, regs | 0x01);
                regs = AudioPort->Codec_GetCodecReg("es7210",0x00);
                ESP_LOGW("es7210 reg","0x%02x",regs);
                AudioPort->Codec_SetCodecReg("es7210", 0x00, regs | 0x06);
                ESP_ERROR_CHECK(nvs_commit(my_handle));
                nvs_close(my_handle); 
                vTaskDelay(pdMS_TO_TICKS(300));
                AudioPort->Codec_SetCodecReg("es8311", 0xfa, 0x00);
                AudioPort->Codec_SetCodecReg("es7210", 0x00, 0x32);

                even = xEventGroupWaitBits(UserButtonGroups, set_bit_button(6), pdTRUE, pdFALSE, portMAX_DELAY);
                if(get_bit_button(even, 6)){
                    esp_restart();
                }
            }
        } else if (get_bit_button(even, 1)) { 
            count = 0;
            Mode++;
            if (Mode > 3) {
                Mode = 1;
            }
            printf("button_user 3, Mode=%d\n",Mode);
            if (Mode == 1) {
                // Basic mode
                xEventGroupSetBits(audio_groups, set_bit_button(1));
            } else if (Mode == 2) {
                // network mode
                xEventGroupSetBits(audio_groups, set_bit_button(2));
            } else if (Mode == 3) {
                // xiaozhi mode
                xEventGroupSetBits(audio_groups, set_bit_button(3));
            }
        }
        count++;
        if(count > Mode_Power_Time) {
        }

    }
}

static void audio_user_Task(void *arg)
{
    AudioPort->Codec_PlayInfoAudio();
    int         value = 0;
    int         bytes_write = 0;
    int         bytes_sizt  = 0;
    uint8_t     *Music_ptr ;
    EventBits_t even ;

    const float gain = 4.0f;

    for (;;) {
        even = xEventGroupWaitBits(audio_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(3000));
        if (get_bit_button(even, 0)) {
            value = 0;
        } else if (get_bit_button(even, 1)) {
            value = 1;
        } else if (get_bit_button(even, 2)) {
            value = 2;
        } else if (get_bit_button(even, 3)) {
            value = 3;
        }
        bytes_write = 0;
        bytes_sizt  = AudioPort->Codec_GetMusicSizt(value);
        Music_ptr   = AudioPort->Codec_GetMusicData(value);
        do {
            int16_t sample = *(int16_t*)Music_ptr;

            int32_t sample_32 = (int32_t)sample * gain;
            if (sample_32 > 32767)  sample_32 = 32767;
            if (sample_32 < -32768) sample_32 = -32768;

            sample = (int16_t)sample_32;

            AudioPort->Codec_PlayBackWrite(&sample, 2);

            Music_ptr += 2;
            bytes_write += 2;

            even = xEventGroupWaitBits(audio_groups, set_bit_all, pdFALSE, pdFALSE, pdMS_TO_TICKS(1));
            if (even) break;

        } while (bytes_write < bytes_sizt);
    }
}

void Mode_Selection_Init(void) {

    AudioPort    = new CodecPort(I2cBus);
    audio_groups = xEventGroupCreate();
    xEventGroupSetBits(audio_groups, set_bit_button(0));
    xTaskCreate(mode_selection_user_Task, "mode_selection_user_Task", 4 * 1024, NULL, 3, NULL);
    xTaskCreate(audio_user_Task, "audio_user_Task", 4 * 1024, NULL, 3, NULL);
}