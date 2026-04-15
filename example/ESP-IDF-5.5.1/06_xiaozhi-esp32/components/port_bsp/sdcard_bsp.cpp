#include <stdio.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include "sdcard_bsp.h"
#include "esp_vfs.h"
#include <string.h>
#include <errno.h>      // errno/EEXIST 错误码
#include "esp_err.h"    // ESP错误码（如ESP_OK/ESP_FAIL）

CustomSDPort::CustomSDPort(const char *SdName,int clk,int cmd,int d0,int d1,int d2,int d3,int width) :
SdName_(SdName)
{
    ScanListHandle = list_new();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed           = false;
    mount_config.max_files                        = 5;
    mount_config.allocation_unit_size             = 16 * 1024 * 3;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width               = width;
    slot_config.clk                 = (gpio_num_t)clk;
    slot_config.cmd                 = (gpio_num_t)cmd;
    slot_config.d0                  = (gpio_num_t)d0;
    slot_config.d1                  = (gpio_num_t)d1;
    slot_config.d2                  = (gpio_num_t)d2;
    slot_config.d3                  = (gpio_num_t)d3;

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SdName_, &host, &slot_config, &mount_config, &sdcard_host));

    if (sdcard_host != NULL) {
        sdmmc_card_print_info(stdout, sdcard_host);
        is_SdcardInitOK = 1;
    } else {
        is_SdcardInitOK = 0;
    }
}

CustomSDPort::~CustomSDPort() {

}

int CustomSDPort::SDPort_WriteFile(const char *path, const void *data, size_t data_len) {
    if (sdcard_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(sdcard_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    // ========== 标准mkdir创建目录（兼容所有ESP-IDF版本） ==========
    char dir_path[256] = {0};
    const char *last_slash = strrchr(path, '/'); // 找到最后一个/
    if (last_slash != NULL && (last_slash - path) > 0) {
        // 提取目录路径（如 /sdcard/img/）
        size_t dir_len = last_slash - path + 1;
        if (dir_len < sizeof(dir_path)) {
            strncpy(dir_path, path, dir_len);
            dir_path[dir_len] = '\0'; // 确保字符串结束

            // 标准POSIX mkdir：0777是权限（ESP32中可忽略）
            int mkdir_ret = mkdir(dir_path, 0777);
            // 仅当目录不存在且创建失败时报错（EEXIST表示目录已存在）
            if (mkdir_ret != 0 && errno != EEXIST) {
                ESP_LOGE(TAG, "Create dir failed: %s, errno=%d", dir_path, errno);
            }
        }
    }

    // 打开文件（目录已确保存在）
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s, errno=%d", path, errno);
        return ESP_ERR_NOT_FOUND;
    }

    // 写入数据
    size_t written = fwrite(data, 1, data_len, f);
    fclose(f); // 强制关闭文件

    if (written != data_len) {
        ESP_LOGE(TAG, "Write incomplete: %zu/%zu bytes", written, data_len);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Write success: %s (len=%zu)", path, data_len);
    return ESP_OK;
}

int CustomSDPort::SDPort_ReadFile(const char *path, uint8_t *buffer, size_t *outLen) {
    if (sdcard_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(sdcard_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    if (file_size <= 0) {
        ESP_LOGE(TAG, "Invalid file size");
        fclose(f);
        return ESP_FAIL;
    }
    fseek(f, 0, SEEK_SET);

    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);

    if (outLen) *outLen = bytes_read;
    return (bytes_read > 0) ? ESP_OK : ESP_FAIL;
}

int CustomSDPort::SDPort_ReadOffset(const char *path, void *buffer, size_t len, size_t offset) {
    if (sdcard_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(sdcard_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, offset, SEEK_SET);
    size_t bytes_read = fread(buffer, 1, len, f);
    fclose(f);
    return bytes_read;
}

int CustomSDPort::SDPort_WriteOffset(const char *path, const void *data, size_t len, bool append) {
    if (sdcard_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(sdcard_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    const char *mode = append ? "ab" : "wb";
    FILE *f = fopen(path, mode);
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t bytes_written = fwrite(data, 1, len, f);
    fclose(f);

    if (!append && len == 0) {
        ESP_LOGI(TAG, "File cleared: %s", path);
        return ESP_OK;
    }
    return bytes_written;
}

sdmmc_card_t* CustomSDPort::SDPort_GetSdMMCHost() {
    return sdcard_host;
}

int CustomSDPort::SDPort_GetScanListValue(void) {
    int              Quantity = 0;
    list_iterator_t *it       = list_iterator_new(ScanListHandle, LIST_HEAD); 
    list_node_t     *node     = list_iterator_next(it);
    while (node != NULL) {
        CustomSDPortNode_t *sdcard_node = (CustomSDPortNode_t *) node->val;
        ESP_LOGI(TAG, "File: %s", sdcard_node->sdcard_name);
        node = list_iterator_next(it);
        Quantity++;
    }
    list_iterator_destroy(it); 
    return Quantity;
}

void CustomSDPort::SDPort_ScanListDir(const char *path) {
    struct dirent *entry;
    DIR           *dir = opendir(path);

    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) { 
            ESP_LOGI(TAG, "Directory: %s", entry->d_name);
        } else {
            if(strstr(entry->d_name,"sys_decode.bmp")) {   //这个文件是jpg或者png转码成bmp的,不需要加入列表
                continue;
            }
            if (strstr(entry->d_name, ".bmp") || strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".jpeg") || strstr(entry->d_name, ".png") \
                || strstr(entry->d_name, ".BMP") || strstr(entry->d_name, ".JPG") || strstr(entry->d_name, ".JPEG") || strstr(entry->d_name, ".PNG")) {
                uint16_t       Namestrlen   = strlen(path) + strlen(entry->d_name) + 1 + 1; 
                if (Namestrlen >= 80) {
                    ESP_LOGE(TAG, "scan file fill _strlen:%d", Namestrlen);
                    continue;
                }
                CustomSDPortNode_t *node_data = (CustomSDPortNode_t *) LIST_MALLOC(sizeof(CustomSDPortNode_t));
                assert(node_data);
                snprintf(node_data->sdcard_name, sizeof(node_data->sdcard_name), "%s/%s", path, entry->d_name); 
                list_rpush(ScanListHandle, list_node_new(node_data)); 
                ESP_LOGW("Scan_Dir","DirDoc:%s,size:%d",node_data->sdcard_name,strlen(node_data->sdcard_name));
                ImgValue++;
            }                                     
        }
    }
    closedir(dir);
}

list_t* CustomSDPort::SDPort_GetListHost() {
    return ScanListHandle;
}

int CustomSDPort::SDPort_GetSdcardInitOK() {
    return is_SdcardInitOK;
}

void CustomSDPort::SDPort_SetCurrentlyNode(list_node_t *node) {
    CurrentlyNode = node;
}

list_node_t* CustomSDPort::SDPort_GetCurrentlyNode(void) {
    return CurrentlyNode;
}

uint16_t CustomSDPort::Get_Sdcard_ImgValue(void) {
    return ImgValue;
}
