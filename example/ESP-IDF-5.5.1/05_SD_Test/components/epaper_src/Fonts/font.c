#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "FONT";

extern const unsigned char TEMP_str_12[];
extern const unsigned char TEMP_str_16[];
extern const unsigned char TEMP_str_18[];
extern const unsigned char TEMP_str_24[];
extern const unsigned char TEMP_str_28[];
extern const unsigned char TEMP_str_36[];
extern const unsigned char TEMP_str_48[];

// Predefined font instance
cFONT Font12CN = {
    .font_name_EN = Font12EN,
    .font_name_CH = UTF_Font12CH,
    .font_name_CH_ASICC = UTF_Font12CH_ASICC,
    .Width_EN = font12_Width_EN,
    .Width_CH = font12_Width_CH,
    .Height = font12_Height,
    .size_EN = font12_size_EN,
    .size_CH = font12_size_CH,
    .encoding = FONT_ENCODING_UTF8
};

cFONT Font16CN = {
    .font_name_EN = Font16EN,
    .font_name_CH = UTF_Font16CH,
    .font_name_CH_ASICC = UTF_Font16CH_ASICC,
    .Width_EN = font16_Width_EN,
    .Width_CH = font16_Width_CH,
    .Height = font16_Height,
    .size_EN = font16_size_EN,
    .size_CH = font16_size_CH,
    .encoding = FONT_ENCODING_UTF8
};

cFONT Font18CN = {
    .font_name_EN = Font18EN,
    .font_name_CH = UTF_Font18CH,
    .font_name_CH_ASICC = UTF_Font18CH_ASICC,
    .Width_EN = font18_Width_EN,
    .Width_CH = font18_Width_CH,
    .Height = font18_Height,
    .size_EN = font18_size_EN,
    .size_CH = font18_size_CH,
    .encoding = FONT_ENCODING_UTF8
};

cFONT Font24CN = {
    .font_name_EN = Font24EN,
    .font_name_CH = UTF_Font24CH,
    .font_name_CH_ASICC = UTF_Font24CH_ASICC,
    .Width_EN = font24_Width_EN,
    .Width_CH = font24_Width_CH,
    .Height = font24_Height,
    .size_EN = font24_size_EN,
    .size_CH = font24_size_CH,
    .encoding = FONT_ENCODING_UTF8
};

cFONT Font28CN = {
    .font_name_EN = Font28EN,
    .font_name_CH = UTF_Font28CH,
    .font_name_CH_ASICC = UTF_Font28CH_ASICC,
    .Width_EN = font28_Width_EN,
    .Width_CH = font28_Width_CH,
    .Height = font28_Height,
    .size_EN = font28_size_EN,
    .size_CH = font28_size_CH,
    .encoding = FONT_ENCODING_UTF8
};

cFONT Font36CN = {
    .font_name_EN = Font36EN,
    .font_name_CH = UTF_Font36CH,
    .font_name_CH_ASICC = UTF_Font36CH_ASICC,
    .Width_EN = font36_Width_EN,
    .Width_CH = font36_Width_CH,
    .Height = font36_Height,
    .size_EN = font36_size_EN,
    .size_CH = font36_size_CH,
    .encoding = FONT_ENCODING_UTF8
};

cFONT Font48CN = {
    .font_name_EN = Font48EN,
    .font_name_CH = UTF_Font48CH,
    .font_name_CH_ASICC = UTF_Font48CH_ASICC,
    .Width_EN = font48_Width_EN,
    .Width_CH = font48_Width_CH,
    .Height = font48_Height,
    .size_EN = font48_size_EN,
    .size_CH = font48_size_CH,
    .encoding = FONT_ENCODING_UTF8
};

// Pure English definition
sFONT Font12 = {
    .font_name = Font12EN,
    .Width = font12_Width_EN,
    .Height = font12_Height,
    .size = font12_size_EN
};

sFONT Font16 = {
    .font_name = Font16EN,
    .Width = font16_Width_EN,
    .Height = font16_Height,
    .size = font16_size_EN
};

sFONT Font18 = {
    .font_name = Font18EN,
    .Width = font18_Width_EN,
    .Height = font18_Height,
    .size = font18_size_EN
};

sFONT Font24 = {
    .font_name = Font24EN,
    .Width = font24_Width_EN,
    .Height = font24_Height,
    .size = font24_size_EN
};

sFONT Font28 = {
    .font_name = Font28EN,
    .Width = font28_Width_EN,
    .Height = font28_Height,
    .size = font28_size_EN
};

sFONT Font36 = {
    .font_name = Font36EN,
    .Width = font36_Width_EN,
    .Height = font36_Height,
    .size = font36_size_EN
};

sFONT Font48 = {
    .font_name = Font48EN,
    .Width = font48_Width_EN,
    .Height = font48_Height,
    .size = font48_size_EN
};


// character extraction
int Get_Char_Font_Data(cFONT* font, const char* character, unsigned char* buffer)
{
    FILE* file = NULL;
    uint32_t font_offset = 0;
    int char_len = 1;

    // ASCII Character Processing (Half-width)
    if (char_len == 1 && (unsigned char)character[0] < 0x80) {
        // ASCII characters start from 0x20(" ")
        if ((unsigned char)character[0] < 0x20) {
            ESP_LOGW(TAG, "Unsupported ASCII control characters: 0x%02X", (unsigned char)character[0]);
            return -1;
        }
        
        font_offset = ((unsigned char)character[0] - 0x20) * font->size_EN;
        file = fopen(font->font_name_EN, "rb");
        if (!file) {
            ESP_LOGE(TAG, "The English font file cannot be opened: %s", font->font_name_EN);
            return -1;
        }
        
        fseek(file, font_offset, SEEK_SET);
        size_t read_size = fread(buffer, 1, font->size_EN, file);
        fclose(file);
        
        if (read_size != font->size_EN) {
            ESP_LOGE(TAG, "Failed to read ASCII character data");
            return -1;
        }
        
        // ESP_LOGD(TAG, "Read half-width ASCII characters: '%c'(0x%02X), offset=%lu", character[0], (unsigned char)character[0], (unsigned long)font_offset);
        return font->size_EN;
    }
    
    // Multi-byte character processing
    if (font->encoding == FONT_ENCODING_UTF8) {
        int char_len_check = 0;
        uint32_t unicode = UTF8_To_Unicode(character, &char_len_check);
        
        // Determine whether it is full-width ASCII or Chinese characters
        if (unicode >= 0xFF01 && unicode <= 0xFF5E) {
            // Full-width ASCII characters (! "~) 
            font_offset = (unicode - 0xFF00) * font->size_CH;
            file = fopen(font->font_name_CH_ASICC, "rb");
            // ESP_LOGI(TAG, "UTF-8 full-width ASCII: unicode=0x%X", (unsigned int)unicode);
        } else if (unicode >= 0x4E00 && unicode <= 0x9FFF) {
            // Common Chinese Character Zone
            font_offset = (unicode - 0x4E00) * font->size_CH;
            file = fopen(font->font_name_CH, "rb");
            // ESP_LOGI(TAG, "UTF-8 Chinese characters: unicode=0x%X", (unsigned int)unicode);
        } else {
            // ESP_LOGW(TAG, "UTF-8 characters are beyond the supported range: unicode=0x%X", (unsigned int)unicode);
            if(unicode == 0x2103){     // ℃符号
                if (font == &Font12CN) {
                    for (size_t i = 0; i < font->size_CH; i++)
                        buffer[i] = TEMP_str_12[i];
                } else if (font == &Font16CN) {
                    for (size_t i = 0; i < font->size_CH; i++)
                        buffer[i] = TEMP_str_16[i];
                } else if (font == &Font18CN) {
                    for (size_t i = 0; i < font->size_CH; i++)
                        buffer[i] = TEMP_str_18[i];
                } else if (font == &Font24CN) {
                    for (size_t i = 0; i < font->size_CH; i++)
                        buffer[i] = TEMP_str_24[i];
                } else if (font == &Font28CN) {
                    for (size_t i = 0; i < font->size_CH; i++)
                        buffer[i] = TEMP_str_28[i];
                } else if (font == &Font36CN) {
                    for (size_t i = 0; i < font->size_CH; i++)
                        buffer[i] = TEMP_str_36[i];
                } else if (font == &Font48CN) {
                    for (size_t i = 0; i < font->size_CH; i++)
                        buffer[i] = TEMP_str_48[i];
                } else {
                    // 
                }
                return font->size_CH;
            }
            return -1;
        }
        
    }
    
    if (!file) {
        ESP_LOGE(TAG, "The font file cannot be opened");
        return -1;
    }
    
    fseek(file, font_offset, SEEK_SET);
    size_t read_size = fread(buffer, 1, font->size_CH, file);
    fclose(file);
    
    if (read_size != font->size_CH) {
        ESP_LOGE(TAG, "Failed to read character data");
        return -1;
    }
    
    // ESP_LOGD(TAG, "The character reading was successful");
    return font->size_CH;
}

// Obtain the UTF-8 character length
int Get_UTF8_Char_Length(unsigned char first_byte)
{
    if (first_byte < 0x80) return 1;      // ASCII
    if ((first_byte & 0xE0) == 0xC0) return 2; // 110xxxxx
    if ((first_byte & 0xF0) == 0xE0) return 3; // 1110xxxx
    if ((first_byte & 0xF8) == 0xF0) return 4; // 11110xxx
    return 1; 
}

// UTF-8 to Unicode
uint32_t UTF8_To_Unicode(const char* utf8_char, int* char_len)
{
    uint32_t unicode = 0;
    unsigned char* bytes = (unsigned char*)utf8_char;
    
    *char_len = Get_UTF8_Char_Length(bytes[0]);
    
    switch(*char_len) {
        case 1:
            unicode = bytes[0];
            break;
        case 2:
            unicode = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
            break;
        case 3:
            unicode = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
            break;
        case 4:
            unicode = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | 
                     ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
            break;
    }
    
    return unicode;
}

