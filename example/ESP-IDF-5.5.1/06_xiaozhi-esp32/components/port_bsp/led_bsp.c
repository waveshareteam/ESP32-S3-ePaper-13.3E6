#include "led_bsp.h"
#include "button_bsp.h"
#include "driver/gpio.h"
#include <stdio.h>

EventGroupHandle_t pwr_led_groups;
EventGroupHandle_t act_led_groups;

void led_set(uint8_t led, uint8_t mode) {
    gpio_set_level(led, mode);
}

void led_PWR_loop_task(void *arg) {
    for (;;) {
        // EventBits_t even = xEventGroupWaitBits(pwr_led_groups, set_bit_button(0) | set_bit_button(1) | set_bit_button(2) | set_bit_button(3) | set_bit_button(4), pdFALSE, pdFALSE, pdMS_TO_TICKS(10));
        EventBits_t even = xEventGroupGetBits(pwr_led_groups);
        if (get_bit_data(even, 0)) { //常灭
            led_set(LED_PIN_PWR, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 1)) { //常亮
            led_set(LED_PIN_PWR, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 2)) { //慢闪
            led_set(LED_PIN_PWR, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_set(LED_PIN_PWR, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 3)) { //中闪
            led_set(LED_PIN_PWR, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_set(LED_PIN_PWR, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (get_bit_data(even, 4)) { //快闪
            led_set(LED_PIN_PWR, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_set(LED_PIN_PWR, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else { //常灭
            led_set(LED_PIN_PWR, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        // 新增：强制让出CPU，让IDLE任务执行
        taskYIELD();
    }
}

void led_ACT_loop_task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupGetBits(act_led_groups);
        if (get_bit_data(even, 0)) { //常灭
            led_set(LED_PIN_ACT, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 1)) { //常亮
            led_set(LED_PIN_ACT, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 2)) { //慢闪
            led_set(LED_PIN_ACT, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_set(LED_PIN_ACT, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 3)) { //中闪
            led_set(LED_PIN_ACT, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_set(LED_PIN_ACT, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (get_bit_data(even, 4)) { //快闪
            led_set(LED_PIN_ACT, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_set(LED_PIN_ACT, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else { //常灭
            led_set(LED_PIN_ACT, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        // 新增：强制让出CPU，让IDLE任务执行
        taskYIELD();
    }
}

/*
  LED闪烁速度
  mode : 
  0     :   常灭
  1     :   常亮
  2     :   慢闪
  3     :   中闪
  4     :   快闪
  其他  :   常闭
*/
void pwr_led_set_Flicker(uint8_t mode) {
    xEventGroupClearBits(pwr_led_groups, set_bit_button(0) | set_bit_button(1) | set_bit_button(2) | set_bit_button(3) | set_bit_button(4));
    switch (mode) {
    case LED_Eternal_Extinction:
        xEventGroupSetBits(pwr_led_groups, set_bit_button(0));
        break;
    case LED_Lighting_Form:
        xEventGroupSetBits(pwr_led_groups, set_bit_button(1));
        break;
    case LED_Slow_Flash:
        xEventGroupSetBits(pwr_led_groups, set_bit_button(2));
        break;
    case LED_Mid_Flash:
        xEventGroupSetBits(pwr_led_groups, set_bit_button(3));
        break;
    case LED_Quick_Flash:
        xEventGroupSetBits(pwr_led_groups, set_bit_button(4));
        break;
    default:
        break;
    }
}
void act_led_set_Flicker(uint8_t mode) {
    xEventGroupClearBits(act_led_groups, set_bit_button(0) | set_bit_button(1) | set_bit_button(2) | set_bit_button(3) | set_bit_button(4));
    switch (mode) {
    case LED_Eternal_Extinction:
        xEventGroupSetBits(act_led_groups, set_bit_button(0));
        break;
    case LED_Lighting_Form:
        xEventGroupSetBits(act_led_groups, set_bit_button(1));
        break;
    case LED_Slow_Flash:
        xEventGroupSetBits(act_led_groups, set_bit_button(2));
        break;
    case LED_Mid_Flash:
        xEventGroupSetBits(act_led_groups, set_bit_button(3));
        break;
    case LED_Quick_Flash:
        xEventGroupSetBits(act_led_groups, set_bit_button(4));
        break;
    default:
        break;
    }
}

void led_init(void) {
    pwr_led_groups          = xEventGroupCreate();
    act_led_groups          = xEventGroupCreate();
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask  = ((uint64_t) 0x01 << LED_PIN_PWR) | ((uint64_t) 0x01 << LED_PIN_ACT);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
    led_set(LED_PIN_PWR, LED_OFF);
    led_set(LED_PIN_ACT, LED_OFF);

    xTaskCreate(led_PWR_loop_task, "led_PWR_loop_task", 2 * 1024, NULL, 3,NULL);
    xTaskCreate(led_ACT_loop_task, "led_ACT_loop_task", 2 * 1024, NULL, 3,NULL);
}