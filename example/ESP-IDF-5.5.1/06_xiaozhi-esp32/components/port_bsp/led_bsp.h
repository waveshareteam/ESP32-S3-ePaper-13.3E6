#ifndef LED_BSP_H
#define LED_BSP_H

#include "freertos/FreeRTOS.h"

#define LED_PIN_PWR         48  // PWR
#define LED_PIN_ACT         47  // ACT
#define LED_ALL_OFF_PIN     255  

#define LED_ON  1
#define LED_OFF 0

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
typedef enum __LED_Flashing_Speed {
    LED_Eternal_Extinction = 0,
    LED_Lighting_Form,
    LED_Slow_Flash,
    LED_Mid_Flash,
    LED_Quick_Flash,
} LED_Flashing_Speed;

extern EventGroupHandle_t pwr_led_groups;
extern EventGroupHandle_t act_led_groups;


#ifdef __cplusplus
extern "C" {
#endif

void led_init(void);
void led_set(uint8_t led,uint8_t mode);
void pwr_led_set_Flicker(uint8_t mode);
void act_led_set_Flicker(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif 

