#include "power_bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"


SemaphoreHandle_t ADC_USB_mutex = NULL; 
SemaphoreHandle_t ADC_BAT_mutex = NULL;
EventGroupHandle_t power_groups;

static const char *TAG = "ADC_DEMO";

/*---------- handle ----------*/
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t cali_usb = NULL;
static adc_cali_handle_t cali_bat = NULL;

/*---------- initialization function ----------*/
static void adc_init(void)
{
    /* Install the ADC1 One-Shot driver */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id  = ADC_UNIT_1,
        .clk_src  = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));

    /* Configuration channels: GPIO8(CH7) and GPIO1(CH0) */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &chan_cfg));

    /* Create a curve calibration handle */
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_usb));
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_bat));
}


static void PWR_GPIO_Init(void)
{
  gpio_config_t gpio_conf = {};
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_OUTPUT;
  gpio_conf.pin_bit_mask = ((uint64_t)0x01<<AUDIO_PWR) | ((uint64_t)0x01<<EPD_PWR);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}


// Test the voltage at the USB
void ADC_USB_Task(void *arg)
{
    int raw;
    int mv;
    while (1) {
        xSemaphoreTake(ADC_USB_mutex, portMAX_DELAY);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw);
        adc_cali_raw_to_voltage(cali_usb, raw, &mv);
        xSemaphoreGive(ADC_USB_mutex);
        ESP_LOGI(TAG, "GPIO8(USB)  raw:%4d  %4d mV", raw, mv);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Test the partial voltage of the battery
void ADC_BAT_Task(void *arg)
{
    int raw;
    int mv;
    while (1) {
        xSemaphoreTake(ADC_BAT_mutex, portMAX_DELAY);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw);
        adc_cali_raw_to_voltage(cali_bat, raw, &mv);
        xSemaphoreGive(ADC_BAT_mutex);
        ESP_LOGI(TAG, "GPIO1(BAT)  raw:%4d  %4d mV", raw, mv);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Obtain the ADC value of the USB, with a maximum of 4095
int Get_USB_ADC(void)
{
    int raw;
    xSemaphoreTake(ADC_USB_mutex, portMAX_DELAY);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw);
    xSemaphoreGive(ADC_USB_mutex);
    ESP_LOGI(TAG, "GPIO8(USB)  raw:%4d", raw);
    return raw;
}
// Obtain the voltage of the USB
int Get_USB_Voltage(void)
{
    int raw;
    int mv;
    xSemaphoreTake(ADC_USB_mutex, portMAX_DELAY);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw);
    xSemaphoreGive(ADC_USB_mutex);
    adc_cali_raw_to_voltage(cali_usb, raw, &mv);
    // ESP_LOGI(TAG, "GPIO8(USB)  raw:%4d  %4d mV", raw, mv);
    return mv;
}

// Obtain the ADC value of the battery
int Get_BAT_ADC(void)
{
    int raw;
    xSemaphoreTake(ADC_BAT_mutex, portMAX_DELAY);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw);
    xSemaphoreGive(ADC_BAT_mutex);
    // ESP_LOGI(TAG, "GPIO1(BAT)  raw:%4d", raw);
    return raw;
}
// Obtain the battery voltage
float  Get_BAT_Voltage(void)
{
    int raw;
    int mv_adc;
    float mv_bat;
    xSemaphoreTake(ADC_BAT_mutex, portMAX_DELAY);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw);
    adc_cali_raw_to_voltage(cali_usb, raw, &mv_adc);
    xSemaphoreGive(ADC_BAT_mutex);
    ESP_LOGI(TAG, "GPIO8(BAT)  raw:%4d  %4d mV", raw, mv_adc);
    mv_bat = 0.001 * mv_adc * 3;
    ESP_LOGI(TAG, "mv_bat:%f", mv_bat);
    return mv_bat;
}

uint8_t Get_BAT_level(void)
{
    float vol = Get_BAT_Voltage(); 
    if (vol <3.0){
        return 0;
    }else if(vol > 4.12){
        return 100;
    } else {
        float level = (vol-3)/(4.12-3)*100; 
        return (uint8_t)level;
    }
}

// Obtain the battery voltage
float measureVBAT(void)
{
    float Voltage=0.0;
    uint16_t result = Get_BAT_Voltage();
    Voltage = result * 3.3 / 1000;
    ESP_LOGI(TAG,"Raw value: 0x%03x, voltage: %f V\n", result, Voltage);
    return Voltage;
}

// Audio power switch
void AUDIO_PWR_ON_Task(void)
{
    gpio_set_level(AUDIO_PWR,1);
}
void AUDIO_PWR_OFF_Task(void)
{
    gpio_set_level(AUDIO_PWR,0);
}

// E-ink screen power switch
void EPD_PWR_ON_Task(void)
{
    gpio_set_level(EPD_PWR,1);
}
void EPD_PWR_OFF_Task(void)
{
    gpio_set_level(EPD_PWR,0);
}

void BAT_CHARGE_STATE_GPIO_Init(void)
{
    gpio_config_t gpio_conf     = {};
    gpio_conf.intr_type         = GPIO_INTR_DISABLE;
    gpio_conf.mode              = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask      = ((uint64_t)0x01<<CHARGE_STATE);
    gpio_conf.pull_down_en      = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en        = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

// 高电平：ETA6098充电完成
// 低电平：ETA6098正在充电（或无电池）
int BAT_CHARGE_STATE(void)
{
    return Read_CHARGE_STATE;
}

void Power_Groups_Clear(void)
{
    xEventGroupClearBits(power_groups, 0x00FFFFFF);
}


// Power initialization
void Power_Init(void)
{
    PWR_GPIO_Init();

    AUDIO_PWR_ON_Task();
    EPD_PWR_ON_Task();


    adc_init();
    // Apply for a mutex lock to prevent re-refreshing
    //ADC_USB_mutex = xSemaphoreCreateMutex();
    ADC_BAT_mutex = xSemaphoreCreateMutex();
    
    // // Power status bit
    power_groups = xEventGroupCreate();
    
}



















