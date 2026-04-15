#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "epaper_port.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define EPD_PWR         ((gpio_num_t)1)

#define TAG "EPD"
static spi_device_handle_t spi;
static void spi_send_byte(uint8_t cmd);

// Define the data cache area of the e-ink screen
uint8_t *Image_Temporary;

// 13inch3 E6
// Initialize the code array
const uint8_t PSR_V[2] = {
	0xDF, 0x69
};
const uint8_t PWR_V[6] = {
	0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38
};
const uint8_t POF_V[1] = {
	0x00
};
const uint8_t DRF_V[1] = {
	0x00
};
const uint8_t CDI_V[1] = {
	0xF7
};
const uint8_t TCON_V[2] = {
	0x03, 0x03
};
const uint8_t TRES_V[4] = {
	0x04, 0xB0, 0x03, 0x20
};
const uint8_t CMD66_V[6] = {
	0x49, 0x55, 0x13, 0x5D, 0x05, 0x10
};
const uint8_t EN_BUF_V[1] = {
	0x07
};
const uint8_t CCSET_V[1] = {
	0x01
};
const uint8_t PWS_V[1] = {
	0x22
};
const uint8_t AN_TM_V[9] = {
	0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55
};
const uint8_t AGID_V[1] = {
	0x10
};
const uint8_t BTST_P_V[2] = {
	0xE8, 0x28
};
const uint8_t BOOST_VDDP_EN_V[1] = {
	0x01
};
const uint8_t BTST_N_V[2] = {
	0xE8, 0x28
};
const uint8_t BUCK_BOOST_VDDN_V[1] = {
	0x01
};
const uint8_t TFT_VCOM_POWER_V[1] = {
	0x02
};


static void EPD_GPIO_Init(void)
{
  gpio_config_t gpio_conf = {};
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_OUTPUT;
  gpio_conf.pin_bit_mask = ((uint64_t)0x01<<EPD_RST_PIN) | ((uint64_t)0x01<<EPD_DC_PIN) | ((uint64_t)0x01<<EPD_CS_M_PIN) | ((uint64_t)0x01<<EPD_CS_S_PIN) | ((uint64_t)0x01<<EPD_PWR);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));


  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_INPUT;
  gpio_conf.pin_bit_mask = ((uint64_t)0x01<<EPD_BUSY_PIN);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

  EPD_RST_1;
  EPD_CS_M_1;
  EPD_CS_S_1;
  EPD_PWR_OFF;
  gpio_set_level(EPD_SCLK_PIN, 0);
}


void EPD_Port_Init(void)
{
  esp_err_t ret;
  spi_bus_config_t buscfg = 
  {
    .miso_io_num = -1,
    .mosi_io_num = EPD_MOSI_PIN,
    .sclk_io_num = EPD_SCLK_PIN,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = EPD_WIDTH * EPD_HEIGHT, 
  };
  spi_device_interface_config_t devcfg = 
  {
    .spics_io_num = -1,
    .clock_speed_hz = 10 * 1000 * 1000,  //Clock out at 10 MHz
    .mode = 0,                           //SPI mode 0
    .queue_size = 7,                     //We want to be able to queue 7 transactions at a time
  };
  ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  ret = spi_bus_add_device(SPI3_HOST, &devcfg, &spi);
  ESP_ERROR_CHECK(ret);

  EPD_GPIO_Init();
}


// Batch sending of data
esp_err_t spi_send_data(const uint8_t *data, size_t data_size) {
    esp_err_t ret;
    
    const size_t chunk_size = 1024;
    for (size_t i = 0; i < data_size; i += chunk_size) {
        size_t chunk_len = (i + chunk_size > data_size) ? (data_size - i) : chunk_size;
        
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));  // Important: Reset all fields
        
        t.length = chunk_len * 8;    // Sending length (bits)
        t.rxlength = 0;              // Add: The receiving length is 0
        t.tx_buffer = data + i;
        t.rx_buffer = NULL;          // Add: No receiving buffer is required
        
        ret = spi_device_polling_transmit(spi, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmission failed: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    return ESP_OK;
}

static void spi_send_byte(const uint8_t cmd)
{
  esp_err_t ret;
  spi_transaction_t t; 
  memset(&t, 0, sizeof(t));
  t.length = 8;      
  t.tx_buffer = &cmd;
  ret = spi_device_polling_transmit(spi, &t); //Transmit!
  assert(ret == ESP_OK);              //Should have had no issues.
}

/*Pinout of the 13.3-inch e-Paper module by Waveshare Electronics*/

/*
  Ink screen reset
*/
static void EPD_Reset(void)
{
    EPD_RST_1;
    vTaskDelay(pdMS_TO_TICKS(30));
    EPD_RST_0;
    vTaskDelay(pdMS_TO_TICKS(30));
    EPD_RST_1;
    vTaskDelay(pdMS_TO_TICKS(30));
    EPD_RST_0;
    vTaskDelay(pdMS_TO_TICKS(30));
    EPD_RST_1;
    vTaskDelay(pdMS_TO_TICKS(30));
}

/*
  Waiting for the idle signal
*/
static void EPD_ReadBusyH(void)
{
    while(1)
    {
        if(ReadBusy){return;}
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
/*
Send byte command
*/
static void EPD_SendCommand(uint8_t Reg)
{
    EPD_DC_0;
    spi_send_byte(Reg);
}
/*
Send byte data
*/
void EPD_SendData(uint8_t Data)
{
    EPD_DC_1;
    spi_send_byte(Data);
}

void EPD_SendData_Buffer(const uint8_t *buffer, size_t length) 
{
    EPD_DC_1;
    
    esp_err_t ret;
    const size_t chunk_size = 4096;
    
    for (size_t i = 0; i < length; i += chunk_size) {
        size_t current_chunk = (i + chunk_size > length) ? (length - i) : chunk_size;
        
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        
        t.length = current_chunk * 8;
        t.rxlength = 0;   
        t.tx_buffer = buffer + i;
        t.rx_buffer = NULL;      
        
        ret = spi_device_polling_transmit(spi, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmission failed: %s", esp_err_to_name(ret));
            return;
        }
    }
    
    ESP_LOGD(TAG, "All %zu bytes transmitted successfully", length);
}


static void EPD_CS_ALL(uint8_t Value)
{
    gpio_set_level(EPD_CS_M_PIN,Value);
    gpio_set_level(EPD_CS_S_PIN,Value);
}

static void EPD_SPI_Send(uint8_t Cmd, const uint8_t *buf, uint32_t Len)
{
    EPD_SendCommand(Cmd);
    EPD_SendData_Buffer(buf,Len);
}

/*
  The data has been uploaded to Buff
*/
static void EPD_TurnOnDisplay(void)
{
    
    ESP_LOGI(TAG,"Write PON");
    EPD_CS_ALL(0);
    EPD_SendCommand(0x04); // POWER_ON
    EPD_CS_ALL(1);
    EPD_ReadBusyH();

    ESP_LOGI(TAG,"Write DRF");
    vTaskDelay(pdMS_TO_TICKS(50));

    EPD_CS_ALL(0);
    EPD_SPI_Send(DRF, DRF_V, sizeof(DRF_V));
    EPD_CS_ALL(1);
    EPD_ReadBusyH();
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG,"Write POF");
    EPD_CS_ALL(0);
    EPD_SPI_Send(POF, POF_V, sizeof(POF_V));
    EPD_CS_ALL(1);
    // EPD_ReadBusyH();
    ESP_LOGI(TAG,"Display Done!!");
}
/*
EPD init
*/
void EPD_Init(void)
{
    // turn on
    EPD_PWR_ON;
    ESP_LOGI(TAG,"EPD_PWR_ON");
    vTaskDelay(pdMS_TO_TICKS(10));

    EPD_Reset();
    EPD_ReadBusyH();

    EPD_CS_M_0;
	EPD_SPI_Send(AN_TM, AN_TM_V, sizeof(AN_TM_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(CMD66, CMD66_V, sizeof(CMD66_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(PSR, PSR_V, sizeof(PSR_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(CDI, CDI_V, sizeof(CDI_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(TCON, TCON_V, sizeof(TCON_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(AGID, AGID_V, sizeof(AGID_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(PWS, PWS_V, sizeof(PWS_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(CCSET, CCSET_V, sizeof(CCSET_V));
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
	EPD_SPI_Send(TRES, TRES_V, sizeof(TRES_V));
    EPD_CS_ALL(1);

    EPD_CS_M_0;
	EPD_SPI_Send(PWR, PWR_V, sizeof(PWR_V));
    EPD_CS_ALL(1);

    EPD_CS_M_0;
	EPD_SPI_Send(EN_BUF, EN_BUF_V, sizeof(EN_BUF_V));
    EPD_CS_ALL(1);

    EPD_CS_M_0;
	EPD_SPI_Send(BTST_P, BTST_P_V, sizeof(BTST_P_V));
    EPD_CS_ALL(1);

    EPD_CS_M_0;
	EPD_SPI_Send(BOOST_VDDP_EN, BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
    EPD_CS_ALL(1);

    EPD_CS_M_0;
	EPD_SPI_Send(BTST_N, BTST_N_V, sizeof(BTST_N_V));
    EPD_CS_ALL(1);

    EPD_CS_M_0;
	EPD_SPI_Send(BUCK_BOOST_VDDN, BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
    EPD_CS_ALL(1);

    EPD_CS_M_0;
	EPD_SPI_Send(TFT_VCOM_POWER, TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
    EPD_CS_ALL(1);

    ESP_LOGI("TAG","OK");
}




/*Shared function API*/

/******************************************************************************
function :  Clear screen
parameter:
******************************************************************************/
void EPD_Clear(uint8_t color)
{
    uint8_t Color;
    Color = (color<<4)|color;
    size_t buffer_size =  EPD_WIDTH * EPD_HEIGHT / 4;
    if((Image_Temporary = (uint8_t *)heap_caps_malloc(buffer_size,MALLOC_CAP_SPIRAM)) == NULL) 
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        // return ESP_FAIL;
        uint32_t Width, Height;
        Width = EPD_WIDTH / 4 ;
        Height = EPD_HEIGHT;

        uint8_t buf[EPD_WIDTH / 4];
        for (uint32_t j = 0; j < Width; j++) {
            buf[j] = Color;
        }

        ESP_LOGE(TAG,"CS_M_0");
        EPD_CS_M_0;
        EPD_SendCommand(0x10);
        for (uint32_t j = 0; j < Height; j++) {
            spi_send_data(buf, Width);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        EPD_CS_ALL(1);

        ESP_LOGE(TAG,"CS_S_0");
        EPD_CS_S_0;
        EPD_SendCommand(0x10);
        for (uint32_t j = 0; j < Height; j++) {
            spi_send_data(buf, Width);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        EPD_CS_ALL(1);

    } else {

        for (uint32_t j = 0; j < buffer_size; j++) {
            Image_Temporary[j] = Color;
        }
        ESP_LOGE(TAG,"CS_M_0");
        EPD_CS_M_0;
        EPD_SendCommand(0x10);
        EPD_SendData_Buffer(Image_Temporary, buffer_size);
        EPD_CS_ALL(1);

        ESP_LOGE(TAG,"CS_S_0");
        EPD_CS_S_0;
        EPD_SendCommand(0x10);
        EPD_SendData_Buffer(Image_Temporary, buffer_size);
        EPD_CS_ALL(1);

        heap_caps_free(Image_Temporary);
    }
    
    EPD_TurnOnDisplay();
}


void EPD_Display(const uint8_t *Image)
{
    size_t buffer_size =  EPD_WIDTH * EPD_HEIGHT / 4;
    if((Image_Temporary = (uint8_t *)heap_caps_malloc(buffer_size,MALLOC_CAP_SPIRAM)) == NULL) 
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        // return ESP_FAIL;
        uint32_t Width, Width1, Height;
        Width = EPD_WIDTH / 2;
        Width1 = EPD_WIDTH / 4;
        Height = EPD_HEIGHT;

        ESP_LOGE(TAG,"CS_M_0");
        EPD_CS_M_0;
        EPD_SendCommand(0x10);
        for(uint32_t i=0; i<Height; i++ )
        {
            spi_send_data(Image + i*Width,Width1);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
            
        EPD_CS_ALL(1);

        ESP_LOGE(TAG,"CS_S_0");
        EPD_CS_S_0;
        EPD_SendCommand(0x10);
        for(uint32_t i=0; i<Height; i++ )
        {
            spi_send_data(Image + i*Width + Width1,Width1);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        EPD_CS_ALL(1);

    } else {
        uint32_t Width = EPD_WIDTH / 4;
        uint32_t Height = EPD_HEIGHT;
        for (uint32_t i = 0; i < Height; i++)
        {
            for (uint32_t j = 0; j < Width; j++)
            {
                Image_Temporary[i*Width+j] = Image[i*Width*2+j];
            }
        }
        ESP_LOGE(TAG,"CS_M_0");
        EPD_CS_M_0;
        EPD_SendCommand(0x10);
        EPD_SendData_Buffer(Image_Temporary, buffer_size);
        EPD_CS_ALL(1);

        for (uint32_t i = 0; i < Height; i++)
        {
            for (uint32_t j = 0; j < Width; j++)
            {
                Image_Temporary[i*Width+j] = Image[i*Width*2+j+Width];
            }
        }
        ESP_LOGE(TAG,"CS_S_0");
        EPD_CS_S_0;
        EPD_SendCommand(0x10);
        EPD_SendData_Buffer(Image_Temporary, buffer_size);
        EPD_CS_ALL(1);

        heap_caps_free(Image_Temporary);
    }
    EPD_TurnOnDisplay();
}


/******************************************************************************
function :  Enter sleep mode
parameter:
******************************************************************************/
void EPD_Sleep(void)
{
    EPD_CS_ALL(0);
    EPD_SendCommand(0x07); // DEEP_SLEEP
    EPD_SendData(0XA5);
    EPD_CS_ALL(1);

    // Turn off the power
    vTaskDelay(pdMS_TO_TICKS(100));
    EPD_PWR_OFF;
}
