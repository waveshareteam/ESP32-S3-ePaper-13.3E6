#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sdcard_bsp.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "_sdcard";

#define SDMMC_D0_PIN    5  
#define SDMMC_D1_PIN    4
#define SDMMC_D2_PIN    16
#define SDMMC_D3_PIN    15
#define SDMMC_CLK_PIN   6
#define SDMMC_CMD_PIN   7

#define SDlist "/sdcard" // Root directory mount point


// Image position, list position, image count position
#define img_list        "/sdcard/imgs"
#define img_path        "/sdcard/fileList.txt"
#define img_index       "/sdcard/index.txt"


sdmmc_card_t *card_host = NULL;


uint8_t _sdcard_init(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
    {
        .format_if_mount_failed = false,     
        .max_files = 5,                        
        .allocation_unit_size = 16 * 1024 *3, 
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;         
    slot_config.clk = SDMMC_CLK_PIN;
    slot_config.cmd = SDMMC_CMD_PIN;
    slot_config.d0 = SDMMC_D0_PIN;
    slot_config.d1 = SDMMC_D1_PIN;
    slot_config.d2 = SDMMC_D2_PIN;
    slot_config.d3 = SDMMC_D3_PIN;

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card_host));

    if(card_host != NULL)
    {
        sdmmc_card_print_info(stdout, card_host);   
        return 1;
    }
    return 0;
}

/**
 * @brief  write data
 * 
 * @param path file address
 * @param data The data to be written
 * @return * esp_err_t 
 */
esp_err_t s_example_write_file(const char *path, char *data)
{
  esp_err_t err;
  if(card_host == NULL)
  {
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card_host);
  if(err != ESP_OK)
  {
    return err;
  }
  FILE *f = fopen(path, "w");
  if(f == NULL)
  {
    printf("path:Write Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fprintf(f, data); //写入
  fclose(f);
  return ESP_OK;
}

/**
 * @brief   reading data
 * 
 * @param path      file address
 * @param pxbuf     The address for saving the read data
 * @param outLen    Read the length of the data
 * @return esp_err_t 
 */
esp_err_t s_example_read_file(const char *path,uint8_t *pxbuf,uint32_t *outLen)
{
  esp_err_t err;
  if(card_host == NULL)
  {
    printf("path:card == NULL\n");
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card_host);
  if(err != ESP_OK)
  {
    printf("path:card == NO\n");
    return err;
  }
  FILE *f = fopen(path, "rb");
  if (f == NULL)
  {
    printf("path:Read Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fseek(f, 0, SEEK_END); 
  uint32_t unlen = ftell(f);
  //fgets(pxbuf, unlen, f); 
  fseek(f, 0, SEEK_SET);
  uint32_t poutLen = fread((void *)pxbuf,1,unlen,f);
  printf("pxlen: %ld,outLen: %ld\n",unlen,poutLen);
  *outLen = poutLen;
  fclose(f);
  return ESP_OK;
}

/**
 * @brief   reading data
 * 
 * @param path      file address
 * @param buffer    The address for saving the read data
 * @param len       Read the length of the data
 * @param offset    File offset address
 * @return esp_err_t 
 */
uint32_t s_example_read_from_offset(const char *path, char *buffer, uint32_t len, uint32_t offset)
{
  esp_err_t err;
  if (card_host == NULL)
  {
    ESP_LOGE(TAG, "SD card not initialized (card == NULL)");
    return 0;
  }
  err = sdmmc_get_status(card_host);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "SD card status check failed (card not present or unresponsive)");
    return 0;
  }
  FILE *f = fopen(path, "rb");
  if (f == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file: %s", path);
    return 0;
  }

  fseek(f, offset, SEEK_SET);
  uint32_t bytesRead = fread((void *)buffer, 1, len, f);
  fclose(f);
  //ESP_LOGI(TAG, "Read %u bytes from file: %s (offset: %u)", bytesRead, path, offset);
  return bytesRead;
}
/**
 * @brief   write data
 * 
 * @param path      file address
 * @param buffer    The data to be written
 * @param len       data length
 * @param offset    File offset address
 * @return esp_err_t 
 */
uint32_t s_example_wriet_from_offset(const char *path, char *buffer, uint32_t len, uint8_t mode)
{
  esp_err_t err;
  if (card_host == NULL)
  {
    ESP_LOGE(TAG, "SD card not initialized (card == NULL)");
    return 0;
  }
  err = sdmmc_get_status(card_host);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "SD card status check failed (card not present or unresponsive)");
    return 0;
  }
  FILE *f = NULL;
  if(mode == 0)
  {
    f = fopen(path, "w");
    if (f == NULL)
    {
      ESP_LOGE(TAG, "Failed to open file: %s", path);
      return 0;
    }
    fclose(f);
    return 0;
  }
  else
  {
    f = fopen(path, "ab");
    if (f == NULL)
    {
      ESP_LOGE(TAG, "Failed to open file: %s", path);
      return 0;
    }
    uint32_t bytesRead = fwrite((void *)buffer, 1, len, f);
    fclose(f);
    return bytesRead;
  }
}


// Read the file and sort it
int scan_imgs_and_save_list()
{
    char *file_names[MAX_FILES];
    int file_count = 0;

    DIR *dir = opendir(img_list);
    if (!dir) {
        ESP_LOGE("sdscan", "Failed to open directory: %s", img_list);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (entry->d_type == DT_REG) {
            file_names[file_count] = strdup(entry->d_name);
            file_count++;
        }
    }
    closedir(dir);

    for (int i = 0; i < file_count - 1; i++) {
        for (int j = i + 1; j < file_count; j++) {
            if (strcmp(file_names[i], file_names[j]) > 0) {
                char *tmp = file_names[i];
                file_names[i] = file_names[j];
                file_names[j] = tmp;
            }
        }
    }

    FILE *list_file = fopen(img_path, "w");
    if (!list_file) {
        ESP_LOGE("sdscan", "Failed to create file: %s", img_path);
        for (int i = 0; i < file_count; i++) free(file_names[i]);
        return -2;
    }
    for (int i = 0; i < file_count; i++) {
        fprintf(list_file, "%s\n", file_names[i]);
        free(file_names[i]);
    }
    fclose(list_file);

    int old_index = 0, old_count = 0;
    FILE *index_file = fopen(img_index, "r");
    bool need_reset = false;
    if (index_file) {
        if (fscanf(index_file, "%d\n%d", &old_index, &old_count) != 2) {
            need_reset = true;
        }
        fclose(index_file);
        if (old_count != file_count) {
            need_reset = true;
        }
    } else {
        need_reset = true;
    }

    index_file = fopen(img_index, "w");
    if (!index_file) {
        ESP_LOGE("sdscan", "Failed to create file: %s", img_index);
        return -3;
    }
    if (need_reset) {
        fprintf(index_file, "0\n%d\n", file_count);
    } else {
        int write_index = (old_index >= file_count || old_index < 0) ? 0 : old_index;
        fprintf(index_file, "%d\n%d\n", write_index, file_count);
    }
    fclose(index_file);

    return file_count;
}

// Read the file list
int load_file_list(const char *list_path, char file_names[MAX_FILES][128], int *file_count)
{
    FILE *f = fopen(list_path, "r");
    if (!f) return -1;
    int count = 0;
    while (count < MAX_FILES && fgets(file_names[count], 128, f)) {
        char *newline = strchr(file_names[count], '\n');
        if (newline) *newline = '\0';
        count++;
    }
    fclose(f);
    *file_count = count;
    return 0;
}

// Read the current index
int load_index(const char *index_path)
{
    FILE *f = fopen(index_path, "r");
    if (!f) return 0;
    int idx = 0;
    fscanf(f, "%d", &idx);
    fclose(f);
    return idx;
}

// Get the current image path
int get_current_img_path(char *out_path, int out_size)
{
    char file_names[MAX_FILES][128];
    int file_count = 0;
    if (load_file_list(img_path, file_names, &file_count) != 0) return -1;
    int idx = load_index(img_index);
    if (idx < 0 || idx >= file_count) return -2;
    snprintf(out_path, out_size, "%s/%s", img_list, file_names[idx]);
    return 0;
}

// Obtain the file name of the index line in fileList.txt and concatenate the complete path to pathName
int get_img_name_by_index(char *pathName, int pathLen)
{
    FILE *index_file = fopen(img_index, "r");
    int targetIndex = 0, file_count = 0;
    if (index_file) {
        fscanf(index_file, "%d\n%d", &targetIndex, &file_count);
        fclose(index_file);
    } else {
        return -1;
    }

    if (file_count == 0) return -2;
    if (targetIndex >= file_count || targetIndex < 0) targetIndex = 0;

    FILE *fil = fopen(img_path, "r");
    if (!fil) return -1;
    int i = 0;
    char fileName[128] = {0};
    while (fgets(fileName, sizeof(fileName), fil)) {
        if (i == targetIndex) {
            char *newline = strchr(fileName, '\n');
            if (newline) *newline = '\0';
            snprintf(pathName, pathLen, "%s/%s", img_list, fileName);
            fclose(fil);

            int newIndex = targetIndex + 1;
            if (newIndex >= file_count) newIndex = 0;
            FILE *index_file2 = fopen(img_index, "w");
            if (index_file2) {
                fprintf(index_file2, "%d\n%d\n", newIndex, file_count);
                fclose(index_file2);
            }
            return 0;
        }
        i++;
    }
    fclose(fil);
    return -2; 
}
