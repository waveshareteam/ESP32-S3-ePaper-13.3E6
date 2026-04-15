#ifndef POWER_BSP_H
#define POWER_BSP_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// 
#define AUDIO_PWR           ((gpio_num_t)13)
#define EPD_PWR             ((gpio_num_t)1)


#define AUDIO_PWR_ON        gpio_set_level(AUDIO_PWR,1)
#define AUDIO_PWR_OFF       gpio_set_level(AUDIO_PWR,0)
#define EPD_PWR_ON          gpio_set_level(EPD_PWR,1)
#define EPD_PWR_OFF         gpio_set_level(EPD_PWR,0)

#define CHARGE_STATE        ((gpio_num_t)38)
#define Read_CHARGE_STATE   gpio_get_level(CHARGE_STATE)

/**
 * 0 : USB 接入，电压正常，使用USB电源供电
 * 1 : USB 未接入，或电压不够，使用电池供电
 * 2 : 电池电量不够用，请充电
 */
extern EventGroupHandle_t power_groups;

// extern EventGroupHandle_t pwr_groups;
#define set_bit_power(x) ((uint32_t)(0x01)<<(x))
#define get_bit_power(x,y) (((uint32_t)(x)>>(y)) & 0x01)
#define clear_bit_power_all 0x00ffffff                   //Up to 24 bits


#ifdef __cplusplus
extern "C" {
#endif

void ADC_USB_Task(void *arg);
void ADC_BAT_Task(void *arg);

int Get_USB_ADC(void);
int Get_USB_Voltage(void);
int Get_BAT_ADC(void);
float Get_BAT_Voltage(void);
uint8_t Get_BAT_level(void);
float measureVBAT(void);

void AUDIO_PWR_ON_Task(void);
void AUDIO_PWR_OFF_Task(void);

void EPD_PWR_ON_Task(void);
void EPD_PWR_OFF_Task(void);

// 高电平：ETA6098充电完成
// 低电平：ETA6098正在充电（或无电池）
int BAT_CHARGE_STATE(void);

void Power_Groups_Clear(void);

void Power_Init(void);

#ifdef __cplusplus
}
#endif
#endif