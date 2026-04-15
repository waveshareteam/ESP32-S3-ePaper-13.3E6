#ifndef EPAPER_PORT_H
#define EPAPER_PORT_H


/**********************************
Color Index 6 Color
**********************************/
#define EPD_BLACK   0x0 
#define EPD_WHITE   0x1 
#define EPD_YELLOW  0x2 
#define EPD_RED     0x3 

#define EPD_BLUE    0x5 
#define EPD_GREEN   0x6 


/**********************************
EPD 13inch3 E6
**********************************/
#define EPD_WIDTH  1200
#define EPD_HEIGHT 1600

#define PSR             0x00
#define PWR             0x01
#define POF             0x02
#define PON             0x04
#define BTST_N          0x05
#define BTST_P          0x06
#define DTM             0x10
#define DRF             0x12
#define CDI             0x50
#define TCON            0x60
#define TRES            0x61
#define AN_TM           0x74
#define AGID            0x86
#define BUCK_BOOST_VDDN 0xB0
#define TFT_VCOM_POWER  0xB1
#define EN_BUF          0xB6
#define BOOST_VDDP_EN   0xB7
#define CCSET           0xE0
#define PWS             0xE3
#define CMD66           0xF0


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


#define EPD_RST_1    gpio_set_level(EPD_RST_PIN,1)
#define EPD_RST_0    gpio_set_level(EPD_RST_PIN,0)
#define EPD_CS_M_1   gpio_set_level(EPD_CS_M_PIN,1)
#define EPD_CS_M_0   gpio_set_level(EPD_CS_M_PIN,0)
#define EPD_CS_S_1   gpio_set_level(EPD_CS_S_PIN,1)
#define EPD_CS_S_0   gpio_set_level(EPD_CS_S_PIN,0)
#define EPD_DC_1     gpio_set_level(EPD_DC_PIN,1)
#define EPD_DC_0     gpio_set_level(EPD_DC_PIN,0)

#define ReadBusy     gpio_get_level(EPD_BUSY_PIN)

#define EPD_PWR_ON      gpio_set_level(EPD_PWR,1)
#define EPD_PWR_OFF     gpio_set_level(EPD_PWR,0)

#ifdef __cplusplus
extern "C" {
#endif

void EPD_Port_Init(void);
void EPD_Init(void);
void EPD_Clear(uint8_t color);
void EPD_Display(const uint8_t *Image);
void EPD_Sleep(void);

#ifdef __cplusplus
}
#endif


#endif // !EPAPER_PORT_H
