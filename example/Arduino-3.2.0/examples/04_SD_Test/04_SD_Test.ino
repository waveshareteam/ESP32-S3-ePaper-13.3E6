#include "EPD_13in3e.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "ImageData.h"
#include "Arduino.h"
#include "SD_MMC.h"
#include "GUI_BMPfile.h"
#include "FS.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#define SD_CLK  6
#define SD_CMD  7
#define SD_D0   5
#define SD_D1   4
#define SD_D2   16
#define SD_D3   15

#define MOUNT_POINT "/sdcard"

String bmpFilePaths[32];//bmp file max number = 32 
int bmpFileCount = 0;  

void setup() {
    
    //Mount SD-Card
    Serial.begin(115200);
    delay(1000); 
    SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);
    if (!SD_MMC.begin("/sdcard", true)) { 
        Serial.println("SD card failed to mount\r\n");
        return;
    }
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD card has been successfully mounted(VFS path:%s),sd size:%llu MB\r\n", MOUNT_POINT, cardSize);

    Serial.println("Scan the root directory BMP file...");
    File rootDir = SD_MMC.open("/img");
    if (rootDir && rootDir.isDirectory()) {
        File file = rootDir.openNextFile();
        while (file && bmpFileCount < 32) {
        if (!file.isDirectory()) {
            String fileName = file.name();
            String fullPath = file.path();
            fileName.toLowerCase();
            if (fileName.endsWith(".bmp")) {
                bmpFilePaths[bmpFileCount++] = MOUNT_POINT + fullPath;
                Serial.printf("find BMP:%s\n", fullPath.c_str());
            }
        }
        file.close();
        file = rootDir.openNextFile();
        }
        rootDir.close();
    } else {
        Serial.println("error : failed to open the root directory!");
    }

    if (bmpFileCount == 0) {
        Serial.println("warn : No BMP files were found in the root directory!");
        SD_MMC.end();
        return;
    }
    Serial.printf("A total of %d BMP files \n were found\n", bmpFileCount);

    //Create a new image cache named IMAGE and fill it with white
    printf("EPD_13IN3E_test Demo\r\n");
    DEV_Module_Init();

    printf("e-Paper Init and Clear...\r\n");
    EPD_13IN3E_Init();
    EPD_13IN3E_Clear(EPD_13IN3E_WHITE);
    DEV_Delay_ms(500);

    UBYTE *Image;
    UWORD Imagesize = ((EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1)) * EPD_13IN3E_HEIGHT;
    if((Image = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        while(1);
    }
    Paint_NewImage(Image, EPD_13IN3E_WIDTH, EPD_13IN3E_HEIGHT, 0, EPD_13IN3E_WHITE);
    Paint_SetScale(6);

    for (int i=0; i<bmpFileCount; i++) {
        Serial.printf("Display the %d sheet: %s\n", i+1, bmpFilePaths[i].c_str());
        
        Paint_Clear(EPD_13IN3E_WHITE);
        
        if (GUI_ReadBmp_RGB_6Color(bmpFilePaths[i].c_str(), 0, 0) == 0) {
        EPD_13IN3E_Display(Image);
        } else {
        Serial.println("the file failed to load. Skip!");
        }
        
        DEV_Delay_ms(5000);
    }
    
    EPD_13IN3E_Display(Image6color);
    DEV_Delay_ms(5000);

    Serial.println("Clear...Goto Sleep...\r\n");
    EPD_13IN3E_Clear(EPD_13IN3E_WHITE);
    DEV_Delay_ms(2000);
    EPD_13IN3E_Sleep();
    DEV_Delay_ms(2000);
    // free(Image);
    SD_MMC.end();
    Serial.println("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
}

void loop() {
}