#ifndef __FONT_H_
#define __FONT_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Font encoding type
typedef enum {
    FONT_ENCODING_UTF8 = 0,
    FONT_ENCODING_GBK = 1
} FONT_ENCODING_TYPE;

// Font structure definition
typedef struct
{
    const char *font_name_EN;      // English font file path
    const char *font_name_CH;      // The file path of Chinese fonts
    const char *font_name_CH_ASICC; // File path of Chinese ASCII extended font
    
    uint16_t Width_EN;             // English character width
    uint16_t Width_CH;             // Chinese character width
    uint16_t Height;               // character height
    uint16_t size_EN;              // The number of English character bytes
    uint16_t size_CH;              // The number of Chinese character bytes
    FONT_ENCODING_TYPE encoding;   // encoding type
}cFONT;

// Font structure definition
typedef struct
{
    const char *font_name;      // English font file path
    
    uint16_t Width;             // English character width
    uint16_t Height;            // character height
    uint16_t size;              // The number of English character bytes
}sFONT;

// Definition of English font file path
#define Font12EN "/sdcard/Weather_img/font12EN.FON"
#define Font16EN "/sdcard/Weather_img/font16EN.FON"
#define Font18EN "/sdcard/Weather_img/font18EN.FON"
#define Font24EN "/sdcard/Weather_img/font24EN.FON"
#define Font28EN "/sdcard/Weather_img/font28EN.FON"
#define Font36EN "/sdcard/Weather_img/font36EN.FON"
#define Font48EN "/sdcard/Weather_img/font48EN.FON"

// UTF-8 Chinese font file path
#define UTF_Font12CH            "/sdcard/Weather_img/font12CH.FON"
#define UTF_Font12CH_ASICC      "/sdcard/Weather_img/font12CH_ASICC.FON"
#define UTF_Font16CH            "/sdcard/Weather_img/font16CH.FON"
#define UTF_Font16CH_ASICC      "/sdcard/Weather_img/font16CH_ASICC.FON"
#define UTF_Font18CH            "/sdcard/Weather_img/font18CH.FON"
#define UTF_Font18CH_ASICC      "/sdcard/Weather_img/font18CH_ASICC.FON"
#define UTF_Font24CH            "/sdcard/Weather_img/font24CH.FON"
#define UTF_Font24CH_ASICC      "/sdcard/Weather_img/font24CH_ASICC.FON"
#define UTF_Font28CH            "/sdcard/Weather_img/font28CH.FON"
#define UTF_Font28CH_ASICC      "/sdcard/Weather_img/font28CH_ASICC.FON"
#define UTF_Font36CH            "/sdcard/Weather_img/font36CH.FON"
#define UTF_Font36CH_ASICC      "/sdcard/Weather_img/font36CH_ASICC.FON"
#define UTF_Font48CH            "/sdcard/Weather_img/font48CH.FON"
#define UTF_Font48CH_ASICC      "/sdcard/Weather_img/font48CH_ASICC.FON"

// Font size definition
#define font12_Width_EN         8
#define font12_Width_CH         16
#define font12_Height           21
#define font12_size_EN          (font12_Width_EN * font12_Height / 8)
#define font12_size_CH          (font12_Width_CH * font12_Height / 8)

#define font16_Width_EN         16
#define font16_Width_CH         24
#define font16_Height           28
#define font16_size_EN          (font16_Width_EN * font16_Height / 8)
#define font16_size_CH          (font16_Width_CH * font16_Height / 8)

#define font18_Width_EN         16
#define font18_Width_CH         24
#define font18_Height           31
#define font18_size_EN          (font18_Width_EN * font18_Height / 8)
#define font18_size_CH          (font18_Width_CH * font18_Height / 8)

#define font24_Width_EN         24
#define font24_Width_CH         32
#define font24_Height           41
#define font24_size_EN          (font24_Width_EN * font24_Height / 8)
#define font24_size_CH          (font24_Width_CH * font24_Height / 8)

#define font28_Width_EN         24
#define font28_Width_CH         40
#define font28_Height           48
#define font28_size_EN          (font28_Width_EN * font28_Height / 8)
#define font28_size_CH          (font28_Width_CH * font28_Height / 8)

#define font36_Width_EN         32
#define font36_Width_CH         48
#define font36_Height           62
#define font36_size_EN          (font36_Width_EN * font36_Height / 8)
#define font36_size_CH          (font36_Width_CH * font36_Height / 8)

#define font48_Width_EN         40
#define font48_Width_CH         64
#define font48_Height           83
#define font48_size_EN          (font48_Width_EN * font48_Height / 8)
#define font48_size_CH          (font48_Width_CH * font48_Height / 8)

// Predefined font instance declaration
extern cFONT Font12CN;
extern cFONT Font16CN;
extern cFONT Font18CN;
extern cFONT Font24CN;
extern cFONT Font28CN;
extern cFONT Font36CN;
extern cFONT Font48CN;

// Font definition in English
extern sFONT Font12;
extern sFONT Font16;
extern sFONT Font18;
extern sFONT Font24;
extern sFONT Font28;
extern sFONT Font36;
extern sFONT Font48;

extern const unsigned char TEMP_str_12[];
extern const unsigned char TEMP_str_16[];
extern const unsigned char TEMP_str_18[];
extern const unsigned char TEMP_str_24[];
extern const unsigned char TEMP_str_28[];
extern const unsigned char TEMP_str_36[];
extern const unsigned char TEMP_str_48[];


#ifdef __cplusplus
extern "C" {
#endif

// Font reading function declaration
int Get_Char_Font_Data(cFONT* font, const char* character, unsigned char* buffer);
int Get_UTF8_Char_Length(unsigned char first_byte);
uint32_t UTF8_To_Unicode(const char* utf8_char, int* char_len);

#ifdef __cplusplus
}
#endif

#endif