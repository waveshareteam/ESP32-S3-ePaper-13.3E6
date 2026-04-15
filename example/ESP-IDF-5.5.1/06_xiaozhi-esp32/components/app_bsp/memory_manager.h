#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化
int memory_init(size_t main_buf_size);

// 主 buffer
uint8_t* memory_get_main_buffer(void);
size_t   memory_get_main_buffer_size(void);

// 动态分配
uint8_t* memory_alloc(size_t size);
void  memory_free(void* ptr);

// 能否分配
int memory_can_alloc(size_t size);

// scratch
uint8_t* memory_get_scratch(size_t offset, size_t size);

// 调试
void memory_print_info(void);

#ifdef __cplusplus
}
#endif

#endif

