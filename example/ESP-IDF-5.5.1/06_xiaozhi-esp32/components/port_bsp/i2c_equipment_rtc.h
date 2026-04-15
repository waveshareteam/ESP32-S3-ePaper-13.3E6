#ifndef I2C_EQUIPMENT_RTC_H
#define I2C_EQUIPMENT_RTC_H

#include "i2c_bsp.h"

// 复用原有错误码枚举，保持一致性
typedef enum{
    NO_ERROR_RTC       = 0x00, // 无错误
    ACK_ERROR_RTC      = 0x01, // 无应答错误
    CHECKSUM_ERROR_RTC = 0x02  // 校验和错误（RTC暂未用到，保留兼容）
}etError_RTC;

typedef enum{
    Alarm_Flag_OFF          = 0x00,
    Alarm_Flag_ON           = 0x10,
}Alarm_Flag;

// PCF8563指令/寄存器地址集
typedef enum{
    RTC_CMD_CONTROL1        = 0x00, // 控制寄存器1（0x00复用，读写区分）
    RTC_CMD_CONTROL2        = 0x01, // 控制寄存器2
    RTC_CMD_OFFSE           = 0x02,
    RTC_CMD_RAM_BYTE        = 0x03, 
    RTC_CMD_SECONDS         = 0x04, 
    RTC_CMD_MINUTES         = 0x05, 
    RTC_CMD_HOURS           = 0x06, 
    RTC_CMD_DAYS            = 0x07, 
    RTC_CMD_WEEKDAYS        = 0x08, 
    RTC_CMD_MONTHS          = 0x09, 
    RTC_CMD_YEARS           = 0x0A, 
    RTC_CMD_SECOND_ALARM    = 0x0B, 
    RTC_CMD_MINUTES_ALARM   = 0x0C, 
    RTC_CMD_HOUR_ARARM      = 0x0D,  
    RTC_CMD_DAY_ALARM       = 0x0E,
    RTC_CMD_WEEKDAY_ALARM   = 0x0F,
    RTC_CMD_TIMER_VALUE     = 0x10,
    RTC_CMD_TIMER_MODE      = 0x11,
}etRTCCommands;

// RTC时间结构体
typedef struct {
    uint8_t seconds; // 0-59
    uint8_t minutes; // 0-59
    uint8_t hours;   // 0-23
    uint8_t days;    // 1-31
    uint8_t weekday; // 1-7
    uint8_t month;   // 1-12
    uint8_t year;    // 0-99（对应2000-2099）
}RTCTimeDef;

class RTC_Pcf85063Port
{
private:
    const char *TAG = "PCF85063";
    const uint16_t CRC_POLYNOMIAL = 0x131;  // 保留CRC多项式（兼容框架）
    const uint8_t RTCAddress = 0x51;        // PCF8563 I2C地址
    I2cMasterBus& i2cbus_;                  // I2C总线句柄引用
    i2c_master_dev_handle_t I2c_DevRTC;     // RTC设备句柄

    // 私有方法
    uint8_t BcdToDec(uint8_t bcd);          // BCD转十进制
    uint8_t DecToBcd(uint8_t dec);          // 十进制转BCD
    etError_RTC RTC_WriteReg(uint8_t reg_addr, uint8_t data); // 写单个寄存器
    etError_RTC RTC_ReadReg(uint8_t reg_addr, uint8_t *data); // 读单个寄存器

public:
    RTC_Pcf85063Port(I2cMasterBus& i2cbus);
    ~RTC_Pcf85063Port();

    // 公有接口
    etError_RTC RTC_SoftReset();                // RTC软复位
    etError_RTC RTC_SetTime(RTCTimeDef *time);  // 设置RTC时间
    etError_RTC RTC_GetTime(RTCTimeDef *time);  // 读取RTC时间
    etError_RTC RTC_SetTime_YMD(uint8_t Years,uint8_t Months,uint8_t Days);
    etError_RTC RTC_SetTime_HMS(uint8_t hour,uint8_t minute,uint8_t second);
    etError_RTC RTC_SetTime_RunAlarm(RTCTimeDef *time, RTCTimeDef *alarmTime);
    etError_RTC RTC_Alarm_Time_Enabled(RTCTimeDef *time);
    etError_RTC RTC_Alarm_Time_Disable(void);
    int RTC_Get_Alarm_Flag();
    etError_RTC RTC_Clear_Alarm_Flag();
};

#endif 