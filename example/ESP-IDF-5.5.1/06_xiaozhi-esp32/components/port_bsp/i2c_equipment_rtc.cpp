#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "i2c_equipment_rtc.h"
#include "i2c_bsp.h"

// 构造函数：初始化I2C设备、复位RTC
RTC_Pcf85063Port::RTC_Pcf85063Port(I2cMasterBus& i2cbus) : i2cbus_(i2cbus) {
    // 配置I2C设备参数（完全参考SHTC3的初始化逻辑）
    i2c_master_bus_handle_t I2cMasterBus = i2cbus_.Get_I2cBusHandle();
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address  = RTCAddress;
    dev_cfg.scl_speed_hz    = 100000; // 100KHz（和SHTC3保持一致）
    ESP_ERROR_CHECK(i2c_master_bus_add_device(I2cMasterBus, &dev_cfg, &I2c_DevRTC));

    // RTC初始化流程（复位+清除控制寄存器）
    RTC_SoftReset();
    vTaskDelay(pdMS_TO_TICKS(20)); // 复位后延时（参考SHTC3的20ms）
    ESP_LOGI(TAG, "PCF8563 init success");
}

RTC_Pcf85063Port::~RTC_Pcf85063Port() {
    // 析构函数：按需释放资源（参考SHTC3的空析构）
}

// BCD转十进制（RTC存储格式为BCD）
uint8_t RTC_Pcf85063Port::BcdToDec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 十进制转BCD（写入RTC前转换）
uint8_t RTC_Pcf85063Port::DecToBcd(uint8_t dec) {
    return (((dec / 10) << 4) | (dec % 10)) & 0xFF;
}

// 写单个RTC寄存器（核心I2C写逻辑，参考SHTC3_WriteBuff）
etError_RTC RTC_Pcf85063Port::RTC_WriteReg(uint8_t reg_addr, uint8_t data) {
    uint8_t senBuf[2] = {reg_addr, data}; // 寄存器地址 + 数据
    int err = i2cbus_.i2c_write_buff(I2c_DevRTC, -1, senBuf, 2);
    etError_RTC error = (err == ESP_OK) ? NO_ERROR_RTC : ACK_ERROR_RTC;
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "WriteReg Failure, reg:0x%02x", reg_addr);
    }
    return error;
}

// 读单个RTC寄存器（核心I2C读逻辑，参考SHTC3_ReadBuff）
etError_RTC RTC_Pcf85063Port::RTC_ReadReg(uint8_t reg_addr, uint8_t *data) {
    uint8_t senBuf[1] = {reg_addr}; // 寄存器地址
    uint8_t readBuf[1] = {0};
    int err = i2cbus_.i2c_master_write_read_dev(I2c_DevRTC, senBuf, 1, readBuf, 1);
    etError_RTC error = (err == ESP_OK) ? NO_ERROR_RTC : ACK_ERROR_RTC;
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "ReadReg Failure, reg:0x%02x", reg_addr);
    } else {
        *data = readBuf[0];
    }
    return error;
}

// RTC软复位（通过写入控制寄存器实现，参考SHTC3_SoftReset）
// 适配PCF85063的软复位（不清除时间，仅复位控制逻辑）
etError_RTC RTC_Pcf85063Port::RTC_SoftReset() {
    etError_RTC error;
    uint8_t ctrl1_val, ctrl2_val;

    error = RTC_ReadReg(RTC_CMD_CONTROL1, &ctrl1_val);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read CONTROL1 failed before SoftReset");
        return error;
    }

    ctrl1_val &= ~(1 << 7); 
    error = RTC_WriteReg(RTC_CMD_CONTROL1, ctrl1_val);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Write CONTROL1 SoftReset Failure");
        return error;
    }

    error = RTC_WriteReg(RTC_CMD_CONTROL2, 0x80);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Write CONTROL2 SoftReset Failure");
    }

    uint8_t seconds_reg;
    error = RTC_ReadReg(RTC_CMD_SECONDS, &seconds_reg);
    if (error == NO_ERROR_RTC && (seconds_reg & 0x80)) {
        ESP_LOGW(TAG, "Clock stopped after reset, re-enable");
        RTC_WriteReg(RTC_CMD_SECONDS, seconds_reg & 0x7F); // 清除停止位
    }

    ESP_LOGI(TAG, "SoftReset done - time data preserved");
    return error;
}

// 设置RTC时间——年月日（参考SHTC3_GetTempAndHumiPolling的流程）
etError_RTC RTC_Pcf85063Port::RTC_SetTime_YMD(uint8_t Years,uint8_t Months,uint8_t Days) {
    etError_RTC error;

    if(Years>99)
		Years = 99;
	if(Months>12)
		Months = 12;
	if(Days>31)
		Days = 31;

    error = RTC_WriteReg(RTC_CMD_YEARS, DecToBcd(Years));
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_MONTHS, DecToBcd(Months) & 0x1F);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_DAYS, DecToBcd(Days) & 0x3F);
    if (error != NO_ERROR_RTC) return error;

    return NO_ERROR_RTC;
}

// 设置RTC时间——时分秒（参考SHTC3_GetTempAndHumiPolling的流程）
etError_RTC RTC_Pcf85063Port::RTC_SetTime_HMS(uint8_t hour,uint8_t minute,uint8_t second) {
    etError_RTC error;

    if(hour>23)
		hour = 23;
	if(minute>59)
		minute = 59;
	if(second>59)
		second = 59;

    error = RTC_WriteReg(RTC_CMD_HOURS, DecToBcd(hour) & 0x3F);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_MINUTES, DecToBcd(minute) & 0x7F);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_SECONDS, DecToBcd(second) & 0x7F); 
    if (error != NO_ERROR_RTC) return error;

    return NO_ERROR_RTC;
}

// 设置RTC时间（参考SHTC3_GetTempAndHumiPolling的流程）
etError_RTC RTC_Pcf85063Port::RTC_SetTime(RTCTimeDef *time) {
    etError_RTC error;

    error = RTC_SetTime_YMD(time->year, time->month, time->days);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_SetTime_YMD(time->hours, time->minutes, time->seconds);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_WEEKDAYS, DecToBcd(time->weekday));
    if (error != NO_ERROR_RTC) return error;

    ESP_LOGI(TAG, "SetTime Success: %02d-%02d-%02d %02d:%02d:%02d",
             2000+time->year, time->month, time->days,
             time->hours, time->minutes, time->seconds);
    return NO_ERROR_RTC;
}

// 读取RTC时间（参考SHTC3_GetTempAndHumiPolling的流程）
etError_RTC RTC_Pcf85063Port::RTC_GetTime(RTCTimeDef *time) {
    uint8_t reg_data;
    etError_RTC error;

    error = RTC_ReadReg(RTC_CMD_YEARS, &reg_data);
    if (error != NO_ERROR_RTC) return error;
    time->year = BcdToDec(reg_data);

    error = RTC_ReadReg(RTC_CMD_MONTHS, &reg_data);
    if (error != NO_ERROR_RTC) return error;
    time->month = BcdToDec(reg_data & 0x1F);

    error = RTC_ReadReg(RTC_CMD_DAYS, &reg_data);
    if (error != NO_ERROR_RTC) return error;
    time->days = BcdToDec(reg_data & 0x3F);

    error = RTC_ReadReg(RTC_CMD_HOURS, &reg_data);
    if (error != NO_ERROR_RTC) return error;
    time->hours = BcdToDec(reg_data & 0x3F);

    error = RTC_ReadReg(RTC_CMD_MINUTES, &reg_data);
    if (error != NO_ERROR_RTC) return error;
    time->minutes = BcdToDec(reg_data & 0x7F);

    error = RTC_ReadReg(RTC_CMD_SECONDS, &reg_data);
    if (error != NO_ERROR_RTC) return error;
    time->seconds = BcdToDec(reg_data & 0x7F);

    error = RTC_ReadReg(RTC_CMD_WEEKDAYS, &reg_data);
    if (error != NO_ERROR_RTC) return error;
    time->weekday = BcdToDec(reg_data & 0x07);

    ESP_LOGI(TAG, "GetTime Success: %02d-%02d-%02d %02d:%02d:%02d",
             2000+time->year, time->month, time->days,
             time->hours, time->minutes, time->seconds);
    return NO_ERROR_RTC;
}

etError_RTC RTC_Pcf85063Port::RTC_SetTime_RunAlarm(RTCTimeDef *time, RTCTimeDef *alarmTime) {
    etError_RTC error;

    error = RTC_SetTime_YMD(time->year, time->month, time->days);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_SetTime_YMD(time->hours, time->minutes, time->seconds);
    if (error != NO_ERROR_RTC) return error;

    RTC_Alarm_Time_Enabled(alarmTime);

    ESP_LOGI(TAG, "SetTime Success: %02d-%02d-%02d %02d:%02d:%02d",
             2000+time->year, time->month, time->days,
             time->hours, time->minutes, time->seconds);
    return NO_ERROR_RTC;
}

etError_RTC RTC_Pcf85063Port::RTC_Alarm_Time_Enabled(RTCTimeDef *time) {
    etError_RTC error;

    while(1) {
        if(time->seconds>59)
        {
            time->seconds = time->seconds - 60;
            time->minutes = time->minutes + 1;
        }
        if(time->minutes>59)
        {
            time->minutes = time->minutes - 60;
            time->hours = time->hours + 1;
        }
        if(time->hours>23)
        {
            time->hours = time->hours - 24;
            time->days = time->days + 1;
        }
        if(time->month == 1 || time->month == 3 || time->month == 5 || time->month == 7 || time->month == 8 || time->month == 10 || time->month == 12)
        {
            if(time->days>31)
            {
                time->days = time->days - 31;
            }
        }
        else if(time->month == 2)
        {
            if(time->year%4==0)
            {
                if(time->days>29)
                {
                    time->days = time->days - 29;
                }
            }
            else
            {
                if(time->days>28)
                {
                    time->days = time->days - 28;
                }
            }
        }
        else
        {
            if(time->days>30)
            {
                time->days = time->days - 30;
            }
        }
        if((time->seconds<60) && (time->minutes<60) && (time->hours<24))
            break;
    }

    uint8_t ctrl2_val;

    error = RTC_ReadReg(RTC_CMD_CONTROL2, &ctrl2_val);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read CONTROL1 failed before SoftReset");
        return error;
    }
    ctrl2_val |= 0x80;
    error = RTC_WriteReg(RTC_CMD_CONTROL2, ctrl2_val);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_DAY_ALARM, DecToBcd(time->days) & 0x7F);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_HOUR_ARARM, DecToBcd(time->hours) & 0x7F);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_MINUTES_ALARM, DecToBcd(time->minutes) & 0x7F);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_WriteReg(RTC_CMD_SECOND_ALARM, DecToBcd(time->seconds) & 0x7F);
    if (error != NO_ERROR_RTC) return error;

    ESP_LOGI("ALARM", "SetTime Success: %02d-%02d-%02d %02d:%02d:%02d",
             2000+time->year, time->month, time->days,
             time->hours, time->minutes, time->seconds);
    return NO_ERROR_RTC;
}

etError_RTC RTC_Pcf85063Port::RTC_Alarm_Time_Disable(void) {
    etError_RTC error;

    uint8_t time_Alarm;

    error = RTC_ReadReg(RTC_CMD_HOUR_ARARM, &time_Alarm);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read RTC_CMD_HOUR_ARARM failed before SoftReset");
        return error;
    }
    time_Alarm |= 0x80;
    error = RTC_WriteReg(RTC_CMD_HOUR_ARARM, time_Alarm);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_ReadReg(RTC_CMD_MINUTES_ALARM, &time_Alarm);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read RTC_CMD_MINUTES_ALARM failed before SoftReset");
        return error;
    }
    time_Alarm |= 0x80;
    error = RTC_WriteReg(RTC_CMD_MINUTES_ALARM, time_Alarm);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_ReadReg(RTC_CMD_SECOND_ALARM, &time_Alarm);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read RTC_CMD_SECOND_ALARM failed before SoftReset");
        return error;
    }
    time_Alarm |= 0x80;
    error = RTC_WriteReg(RTC_CMD_SECOND_ALARM, time_Alarm);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_ReadReg(RTC_CMD_DAY_ALARM, &time_Alarm);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read RTC_CMD_DAY_ALARM failed before SoftReset");
        return error;
    }
    time_Alarm |= 0x80;
    error = RTC_WriteReg(RTC_CMD_DAY_ALARM, time_Alarm);
    if (error != NO_ERROR_RTC) return error;

    error = RTC_ReadReg(RTC_CMD_CONTROL2, &time_Alarm);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read RTC_CMD_CONTROL2 failed before SoftReset");
        return error;
    }
    time_Alarm &= 0x7F;
    error = RTC_WriteReg(RTC_CMD_CONTROL2, time_Alarm);
    if (error != NO_ERROR_RTC) return error;


    return NO_ERROR_RTC;
}

int RTC_Pcf85063Port::RTC_Get_Alarm_Flag() {
    etError_RTC error;
    uint8_t ctrl2_val;

    error = RTC_ReadReg(RTC_CMD_CONTROL2, &ctrl2_val);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read CONTROL1 failed before SoftReset");
        return error;
    }

    if((ctrl2_val & 0x40) == 0x40)
        return Alarm_Flag_ON;
	else 
		return Alarm_Flag_OFF;
}

etError_RTC RTC_Pcf85063Port::RTC_Clear_Alarm_Flag() {
    etError_RTC error;
    uint8_t ctrl2_val;

    error = RTC_ReadReg(RTC_CMD_CONTROL2, &ctrl2_val);
    if (error != NO_ERROR_RTC) {
        ESP_LOGE(TAG, "Read CONTROL1 failed before SoftReset");
        return error;
    }
    ctrl2_val &= 0xBF;

    error = RTC_WriteReg(RTC_CMD_CONTROL2, ctrl2_val);
    if (error != NO_ERROR_RTC) return error;
    return NO_ERROR_RTC;
}










