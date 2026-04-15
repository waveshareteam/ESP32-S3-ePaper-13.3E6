#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "pcf85063_bsp.h"
#include "i2c_bsp.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"


static const char *TAG = "PCF85063";


/******************************************************************************
function:	Read one byte of data to EMC2301 via I2C
parameter:  
            Addr: Register address
Info:
******************************************************************************/
static uint8_t PCF85063_Read_Byte(uint8_t Addr)
{
    uint8_t buf;
    i2c_read_buff(PCF85063_handle, Addr, &buf, 1);
	return buf;
}

/******************************************************************************
function:	Send one byte of data to EMC2301 via I2C
parameter:
            Addr: Register address
           Value: Write to the value of the register
Info:
******************************************************************************/
static void PCF85063_Write_Byte(uint8_t Addr, uint8_t Value)
{
	i2c_write_buff(PCF85063_handle, Addr, &Value, 1);
}

int DecToBcd(int val)
{
	return ((val/10)*16 + (val%10)); 
}

int BcdToDec(int val)
{
	return ((val/16)*10 + (val%16));
}


void PCF85063_init()
{
    // Start the oscillator
    uint8_t ctrl1 = PCF85063_Read_Byte(CONTROL_1_REG);
    ctrl1 &= ~(1 << 7); 
    // Software reset data
    PCF85063_Write_Byte(CONTROL_1_REG, ctrl1);
    vTaskDelay(pdMS_TO_TICKS(10));
    // Configure control Register 2 to enable the alarm clock interrupt
    PCF85063_Write_Byte(CONTROL_2_REG, 0x80);

    // Check and clear the STOP position
    int inspect = 0;
    uint8_t seconds_reg;
    while (1) {
        seconds_reg = PCF85063_Read_Byte(SECONDS_REG);
        if ((seconds_reg & 0x80) == 0) {
            break; 
        }
        PCF85063_Write_Byte(SECONDS_REG, seconds_reg & 0x7F);

        vTaskDelay(pdMS_TO_TICKS(100));
        inspect++;
        if (inspect > 50) {
            ESP_LOGE(TAG, "Clock stability unknown or clock stopped");
            break;
        }
    }
    ESP_LOGI(TAG, "PCF85063 RTC Initialization completed");
}

void PCF85063_SetTime_YMD(int Years,int Months,int Days)
{
	if(Years>99)
		Years = 99;
	if(Months>12)
		Months = 12;
	if(Days>31)
		Days = 31;	
	PCF85063_Write_Byte(YEARS_REG  ,DecToBcd(Years));
	PCF85063_Write_Byte(MONTHS_REG ,DecToBcd(Months)&0x1F);
	PCF85063_Write_Byte(DAYS_REG   ,DecToBcd(Days)&0x3F);
}

void PCF85063_SetTime_HMS(int hour,int minute,int second)
{
	if(hour>23)
		hour = 23;
	if(minute>59)
		minute = 59;
	if(second>59)
		second = 59;
	PCF85063_Write_Byte(HOURS_REG  ,DecToBcd(hour)&0x3F);
	PCF85063_Write_Byte(MINUTES_REG,DecToBcd(minute)&0x7F);
	PCF85063_Write_Byte(SECONDS_REG,DecToBcd(second)&0x7F);
}

Time_data PCF85063_GetTime()
{
	Time_data time;
	time.years = BcdToDec(PCF85063_Read_Byte(YEARS_REG));
	time.months = BcdToDec(PCF85063_Read_Byte(MONTHS_REG)&0x1F);
	time.days = BcdToDec(PCF85063_Read_Byte(DAYS_REG)&0x3F);
	time.hours = BcdToDec(PCF85063_Read_Byte(HOURS_REG)&0x3F);
	time.minutes = BcdToDec(PCF85063_Read_Byte(MINUTES_REG)&0x7F);
	time.seconds = BcdToDec(PCF85063_Read_Byte(SECONDS_REG)&0x7F);
    time.week = PCF85063_Read_Byte(WEEKDAYS_REG) & 0x07;
	return time;
}

void PCF85063_alarm_Time_Enabled(Time_data time)
{
    while(1) {
        if(time.seconds>59)
        {
            time.seconds = time.seconds - 60;
            time.minutes = time.minutes + 1;
        }
        if(time.minutes>59)
        {
            time.minutes = time.minutes - 60;
            time.hours = time.hours + 1;
        }
        if(time.hours>23)
        {
            time.hours = time.hours - 24;
            time.days = time.days + 1;
        }
        if(time.months == 1 || time.months == 3 || time.months == 5 || time.months == 7 || time.months == 8 || time.months == 10 || time.months == 12)
        {
            if(time.days>31)
            {
                time.days = time.days - 31;
            }
        }
        else if(time.months == 2)
        {
            if(time.years%4==0)
            {
                if(time.days>29)
                {
                    time.days = time.days - 29;
                }
            }
            else
            {
                if(time.days>28)
                {
                    time.days = time.days - 28;
                }
            }
        }
        else
        {
            if(time.days>30)
            {
                time.days = time.days - 30;
            }
        }
        if((time.seconds<60) && (time.minutes<60) && (time.hours<24))
            break;
    }
    
    // ESP_LOGE(TAG,"%d-%d-%d %d:%d:%d\r\n",time.years,time.months,time.days,time.hours,time.minutes,time.seconds);
	PCF85063_Write_Byte(CONTROL_2_REG, PCF85063_Read_Byte(CONTROL_2_REG)|0x80);	// Alarm on
	PCF85063_Write_Byte(DAY_ALARM_REG, DecToBcd(time.days) & 0x7F);
    PCF85063_Write_Byte(HOUR_ARARM_REG, DecToBcd(time.hours) & 0x7F);
	PCF85063_Write_Byte(MINUTES_ALARM_REG, DecToBcd(time.minutes) & 0x7F);
	PCF85063_Write_Byte(SECOND_ALARM_REG, DecToBcd(time.seconds) & 0x7F);
}

// Turn off the alarm clock
void PCF85063_alarm_Time_Disable() 
{
	PCF85063_Write_Byte(HOUR_ARARM_REG   ,PCF85063_Read_Byte(HOUR_ARARM_REG)|0x80);
	PCF85063_Write_Byte(MINUTES_ALARM_REG,PCF85063_Read_Byte(MINUTES_ALARM_REG)|0x80);
	PCF85063_Write_Byte(SECOND_ALARM_REG ,PCF85063_Read_Byte(SECOND_ALARM_REG)|0x80);
	PCF85063_Write_Byte(DAY_ALARM_REG, PCF85063_Read_Byte(DAY_ALARM_REG)|0x80);
    PCF85063_Write_Byte(CONTROL_2_REG   ,PCF85063_Read_Byte(CONTROL_2_REG)&0x7F);	// Alarm OFF
}

int PCF85063_get_alarm_flag()
{
	if(((PCF85063_Read_Byte(CONTROL_2_REG))&(0x40)) == 0x40)
		return 1;
	else 
		return 0;
}

// Clear the alarm clock flag bit
void PCF85063_clear_alarm_flag()
{
	PCF85063_Write_Byte(CONTROL_2_REG ,PCF85063_Read_Byte(CONTROL_2_REG)&0xBF);
}

void PCF85063_test()
{
	while(1)
	{
		Time_data T;
		T = PCF85063_GetTime();
		ESP_LOGI(TAG,"%d-%d-%d %d:%d:%d",T.years,T.months,T.days,T.hours,T.minutes,T.seconds);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void rtcRunAlarm(Time_data time, Time_data alarmTime)
{
    PCF85063_SetTime_HMS(time.hours, time.minutes, time.seconds);
	PCF85063_SetTime_YMD(time.years, time.months, time.days);

    PCF85063_alarm_Time_Enabled(alarmTime);
}

void PCF85063_SetTime(Time_data time)
{
    PCF85063_SetTime_HMS(time.hours, time.minutes, time.seconds);
	PCF85063_SetTime_YMD(time.years, time.months, time.days);
    PCF85063_Write_Byte(WEEKDAYS_REG, time.week & 0x07);
}


