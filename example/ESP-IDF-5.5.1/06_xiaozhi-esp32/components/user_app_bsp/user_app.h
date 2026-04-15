#pragma once

#include <freertos/FreeRTOS.h>
#include "sdcard_bsp.h"
#include "display_bsp.h"
#include "i2c_bsp.h"
#include "i2c_equipment.h"
#include "i2c_equipment_rtc.h"
#include "imgdecode_app.h"
#include "led_bsp.h"
#include "power_bsp.h"

/**********************************
EPD 13inch3 E6  GPIO config
**********************************/
#define EPD_SCLK_PIN    9
#define EPD_MOSI_PIN    46

#define EPD_CS_M_PIN    10
#define EPD_CS_S_PIN    3

#define EPD_DC_PIN      11

#define EPD_RST_PIN     2
#define EPD_BUSY_PIN    12


/**********************************
EPD 13inch3 E6  resolution ratio
**********************************/
#define EPD_WIDTH       1200
#define EPD_HEIGHT      1600
#define IMG_PIXELS      (EPD_WIDTH * EPD_HEIGHT)
#define IMG_BYTES       (IMG_PIXELS / 2)

#define EPD_WIDTH_MAX   1600
#define EPD_HEIGHT_MAX  1600


/**********************************
I2C config
**********************************/
#define ESP32_SCL_NUM   42
#define ESP32_SDA_NUM   41
#define ESP32_I2C_PORT  0

/**********************************
Sleep wake-up pin
**********************************/
#define ext_wakeup_pin_0 GPIO_NUM_0


extern ImgDecodeDither decdither;
extern CustomSDPort *SDPort;
extern ePaperPort ePaperDisplay;
extern I2cMasterBus I2cBus;
extern RTC_Pcf85063Port *RTCPort;

// 
extern uint8_t Current_mode;
extern uint8_t NetWorkMode; 

uint8_t User_Mode_init(void);       // main.cc

extern SemaphoreHandle_t epaper_gui_semapHandle;
// extern uint8_t Green_led_arg;           
// extern uint8_t Red_led_arg;
extern int img_loopTimer;            
extern EventGroupHandle_t epaper_groups;
extern EventGroupHandle_t ai_IMG_LoopGroup;

void User_xiaozhi_app_init(void); // init
void xiaozhi_init_received(const char *arg1);
void xiaozhi_ai_Message(const char *arg1, const char *arg2);
void xiaozhi_application_received(const char *str);
char* Get_TemperatureHumidity(void);
char* Get_BatteryLevel(void);
void xiaozhi_pwr_OFF(void);
extern int sdcard_bmp_Quantity;
extern int sdcard_doc_count; 
extern int is_ai_img;        
extern bool ai_IMG_LoopTask_Flag;    // 是否开启轮播
extern char ePaper_Refresh_Flag;
extern EventGroupHandle_t ai_IMG_Group;
extern SemaphoreHandle_t ai_img_while_semap;

void User_Basic_mode_app_init(void);
void User_Network_mode_app_init(void);
void Mode_Selection_Init(void);
uint8_t Get_CurrentlyNetworkMode(void);


// Network_mode.cpp
extern uint8_t *img_buf;
// 0 0°/180°  ,   1 90°/270°
extern uint8_t rotary_flag;
// 0 AP  ,   1 STA
extern uint8_t AP_OR_STA_flag;