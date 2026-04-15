#ifndef SDCARD_BSP_H
#define SDCARD_BSP_H
#include "driver/sdmmc_host.h"

extern sdmmc_card_t *card_host;

#define MAX_FILES 100

#ifdef __cplusplus
extern "C" {
#endif

uint8_t _sdcard_init(void);


uint32_t s_example_read_from_offset(const char *path, char *buffer, uint32_t len, uint32_t offset);
uint32_t s_example_wriet_from_offset(const char *path, char *buffer, uint32_t len, uint8_t mode);



int scan_imgs_and_save_list();
int load_file_list(const char *list_path, char file_names[MAX_FILES][128], int *file_count);
int get_current_img_path(char *out_path, int out_size);
int get_img_name_by_index(char *pathName, int pathLen);


#ifdef __cplusplus
}
#endif

#endif