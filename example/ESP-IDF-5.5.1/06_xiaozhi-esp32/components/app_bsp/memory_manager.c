#include "memory_manager.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "MemMgr";

#define img_size 1200*1600*3

static uint8_t* s_main_buf = NULL;
static size_t   s_main_size = 0;

// 分配指针（关键）
static size_t   s_pool_offset_img = 0;
static size_t   s_pool_offset_else = 0;

// 初始化
int memory_init(size_t main_buf_size)
{
    if (s_main_buf) {
        ESP_LOGW(TAG, "Already init");
        return 1;
    }

    ESP_LOGI(TAG, "Alloc main buffer: %u KB", (unsigned int)(main_buf_size / 1024));

    s_main_buf = (uint8_t*)heap_caps_malloc(
        main_buf_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if (!s_main_buf) {
        ESP_LOGE(TAG, "Alloc main buffer failed!");
        return 0;
    }

    s_main_size = main_buf_size;

    s_pool_offset_else = img_size + 200;

    memory_print_info();
    return 1;
}

// 复位指针
void memory_reset_pool_img(void)
{
    s_pool_offset_img = 0;
}

void memory_reset_pool_else(void)
{
    s_pool_offset_else = img_size + 200;
}

// 返回缓存区地址
uint8_t* memory_get_main_buffer(void)
{
    return s_main_buf;
}

// 返回缓存区大小
size_t memory_get_main_buffer_size(void)
{
    return s_main_size;
}

// 申请内存
uint8_t* memory_alloc(size_t size)
{
    void* ptr;
    if(size == img_size){
        if(s_pool_offset_img == 0){
            ptr = s_main_buf;
            s_pool_offset_img = img_size;
        } else {
            ptr = s_main_buf + s_pool_offset_else;
            s_pool_offset_else += size;
        }
    } else {
        // 4字节对齐
        size = (size + 3) & ~3;
        if (s_pool_offset_else + size > s_main_size) {
            ESP_LOGE(TAG, "Pool alloc failed: %u KB", size / 1024);
            return NULL;
        }
        ptr = s_main_buf + s_pool_offset_else;
        s_pool_offset_else += size;
    }
    return ptr;
}


// 释放内存
/*
ESP32-S3 常见内存区域：
DRAM      : 0x3FC8xxxx ~
PSRAM     : 0x3Dxxxxxx ~
IRAM      : 0x4008xxxx ~
*/
// 判断是否属于你的 pool
bool is_in_pool(void* ptr)
{
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t start = (uintptr_t)s_main_buf;
    uintptr_t end = start + s_main_size;
    return (p >= start) && (p < end);
}
// 判断在 pool 的哪个位置
size_t get_offset(void* ptr)
{
    return (uint8_t*)ptr - s_main_buf;
}

void memory_free(void* ptr)
{
    if (is_in_pool(ptr)) {

        size_t offset = get_offset(ptr);

        if (offset == 0) {
            // 释放 img buffer
            memory_reset_pool_img();
        } else {
            // 释放 small pool（直接 reset）
            memory_reset_pool_else();
        }

    } else {
        printf("PSRAM flee");
        heap_caps_free(ptr);
    }
}

// 能否分配
int memory_can_alloc(size_t size)
{
    return s_pool_offset_else + size < s_main_size;
}

// scratch
uint8_t* memory_get_scratch(size_t offset, size_t size)
{
    if (!s_main_buf) return NULL;

    if (offset + size > s_main_size) {
        ESP_LOGE(TAG, "Scratch overflow!");
        return NULL;
    }

    return s_main_buf + offset;
}

void memory_print_info(void)
{
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG, "PSRAM free: %u KB, largest: %u KB",
             (unsigned int)(free_psram / 1024),
             (unsigned int)(largest / 1024));
}

