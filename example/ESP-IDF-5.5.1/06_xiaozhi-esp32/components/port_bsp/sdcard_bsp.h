#pragma once

#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include "list.h"


#define SDMMC_D0_PIN    5
#define SDMMC_D1_PIN    4
#define SDMMC_D2_PIN    16
#define SDMMC_D3_PIN    15
#define SDMMC_CLK_PIN   6
#define SDMMC_CMD_PIN   7

#define SDMMC_width     4


typedef struct
{
    char sdcard_name[80];  
}CustomSDPortNode_t;

class CustomSDPort
{
private:
    const char *TAG = "SDPort";
    const char *SdName_;
    int is_SdcardInitOK = 0;
    sdmmc_card_t *sdcard_host = NULL;
    list_t *ScanListHandle = NULL;

    list_node_t *CurrentlyNode = NULL; 
    uint16_t ImgValue = 0;
public:
    CustomSDPort(const char *SdName,int clk = SDMMC_CLK_PIN,int cmd = SDMMC_CMD_PIN,int d0 = SDMMC_D0_PIN,int d1 = SDMMC_D1_PIN,int d2 = SDMMC_D2_PIN,int d3 = SDMMC_D3_PIN,int width = SDMMC_width);
    ~CustomSDPort();

    int SDPort_WriteFile(const char *path, const void *data, size_t data_len);
    int SDPort_ReadFile(const char *path, uint8_t *buffer, size_t *outLen);
    int SDPort_ReadOffset(const char *path, void *buffer, size_t len, size_t offset);
    int SDPort_WriteOffset(const char *path, const void *data, size_t len, bool append);
    sdmmc_card_t* SDPort_GetSdMMCHost();
    void SDPort_ScanListDir(const char *path);
    list_t* SDPort_GetListHost();
    int SDPort_GetSdcardInitOK();
    int SDPort_GetScanListValue(); 

    void SDPort_SetCurrentlyNode(list_node_t *node);
    list_node_t* SDPort_GetCurrentlyNode(void);
    uint16_t Get_Sdcard_ImgValue(void);
};