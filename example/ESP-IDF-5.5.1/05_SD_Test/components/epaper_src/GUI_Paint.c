/******************************************************************************
* | File      	:   GUI_Paint.c
* | Author      :   Waveshare electronics
* | Function    :	Achieve drawing: draw points, lines, boxes, circles and
*                   their size, solid dotted line, solid rectangle hollow
*                   rectangle, solid circle hollow circle.
* | Info        :
*   Achieve display characters: Display a single character, string, number
*   Achieve time display: adaptive size display time minutes and seconds
*----------------
* |	This version:   V3.2
* | Date        :   2020-07-10
* | Info        :
* -----------------------------------------------------------------------------
* V3.2(2020-07-10):
* 1.Change: Paint_SetScale(UBYTE scale)
*		 Add scale 7 for 5.65f e-Parper
* 2.Change: Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
*		 Add the branch for scale 7
* 3.Change: Paint_Clear(UWORD Color)
*		 Add the branch for scale 7
* -----------------------------------------------------------------------------
* V3.1(2019-10-10):
* 1. Add gray level
*   PAINT Add Scale
* 2. Add void Paint_SetScale(UBYTE scale);
* -----------------------------------------------------------------------------
* V3.0(2019-04-18):
* 1.Change: 
*    Paint_DrawPoint(..., DOT_STYLE DOT_STYLE)
* => Paint_DrawPoint(..., DOT_STYLE Dot_Style)
*    Paint_DrawLine(..., LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
* => Paint_DrawLine(..., DOT_PIXEL Line_width, LINE_STYLE Line_Style)
*    Paint_DrawRectangle(..., DRAW_FILL Filled, DOT_PIXEL Dot_Pixel)
* => Paint_DrawRectangle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
*    Paint_DrawCircle(..., DRAW_FILL Draw_Fill, DOT_PIXEL Dot_Pixel)
* => Paint_DrawCircle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Filll)
*
* -----------------------------------------------------------------------------
* V2.0(2018-11-15):
* 1.add: Paint_NewImage()
*    Create an image's properties
* 2.add: Paint_SelectImage()
*    Select the picture to be drawn
* 3.add: Paint_SetRotate()
*    Set the direction of the cache    
* 4.add: Paint_RotateImage() 
*    Can flip the picture, Support 0-360 degrees, 
*    but only 90.180.270 rotation is better
* 4.add: Paint_SetMirroring() 
*    Can Mirroring the picture, horizontal, vertical, origin
* 5.add: Paint_DrawString_CN() 
*    Can display Chinese(GB1312)   
*
* ----------------------------------------------------------------------------- 
* V1.0(2018-07-17):
*   Create library
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documnetation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to  whom the Software is
* furished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
******************************************************************************/
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset()
#include <math.h>
#include "esp_log.h"

static const char *TAG = "GUI_Paint";

PAINT Paint;

/******************************************************************************
function: Create Image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(UBYTE *image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
    Paint.Image = NULL;
    Paint.Image = image;

    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;    
    Paint.Scale = 2;
    Paint.WidthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
    Paint.HeightByte = Height;    
//    printf("WidthByte = %d, HeightByte = %d\r\n", Paint.WidthByte, Paint.HeightByte);
//    printf(" EPD_WIDTH / 8 = %d\r\n",  122 / 8);
   
    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;
    
    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

/******************************************************************************
function: Select Image
parameter:
    image : Pointer to the image cache
******************************************************************************/
void Paint_SelectImage(UBYTE *image)
{
    Paint.Image = image;
}

/******************************************************************************
function: Select Image Rotate
parameter:
    Rotate : 0,90,180,270
******************************************************************************/
void Paint_SetRotate(UWORD Rotate)
{
    if(Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270) {
        ESP_LOGI(TAG,"Set image Rotate %d", Rotate);
        if(Rotate == ROTATE_90 || Rotate ==  ROTATE_270)
        {
            if(Paint.WidthMemory == Paint.Width) {
                Paint.Width = Paint.HeightMemory;
                Paint.Height = Paint.WidthMemory;
            }
        }
        else {
            if(Paint.WidthMemory != Paint.Width) {
                Paint.Width = Paint.WidthMemory;
                Paint.Height = Paint.HeightMemory;
            }
        }
        Paint.Rotate = Rotate;
    } else {
        ESP_LOGI(TAG,"rotate = 0, 90, 180, 270");
    }
}

/******************************************************************************
function:   Get Image Rotate
parameter:  None
return:     Ratate
******************************************************************************/
UWORD Paint_GetRotate(void)
{
  return Paint.Rotate;
}

/******************************************************************************
function:	Select Image mirror
parameter:
    mirror   :Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint_SetMirroring(UBYTE mirror)
{
    if(mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL || 
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
        ESP_LOGI(TAG,"mirror image x:%s, y:%s",(mirror & 0x01)? "mirror":"none", ((mirror >> 1) & 0x01)? "mirror":"none");
        Paint.Mirror = mirror;
    } else {
        ESP_LOGI(TAG,"mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, \
        MIRROR_VERTICAL or MIRROR_ORIGIN");
    }    
}

void Paint_SetScale(UBYTE scale)
{
    if(scale == 2){
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 8 == 0)? (Paint.WidthMemory / 8 ): (Paint.WidthMemory / 8 + 1);
    }else if(scale == 4){
        Paint.Scale = scale;
        Paint.WidthByte = (Paint.WidthMemory % 4 == 0)? (Paint.WidthMemory / 4 ): (Paint.WidthMemory / 4 + 1);
    }else if(scale == 6 || scale == 7 || scale == 16){
        /* 7 colours are only applicable with 5in65 e-Paper */
        /* 16 colours are used for dithering */
		Paint.Scale = scale;
		Paint.WidthByte = (Paint.WidthMemory % 2 == 0)? (Paint.WidthMemory / 2 ): (Paint.WidthMemory / 2 + 1);;
	}else{
        ESP_LOGI(TAG,"Set Scale Input parameter error");
        ESP_LOGI(TAG,"Scale Only support: 2 4 7 16");
    }
}
/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    if(Xpoint > Paint.Width || Ypoint > Paint.Height){
        ESP_LOGI(TAG,"Exceeding display boundaries");
        return;
    }      
    UWORD X, Y;
    switch(Paint.Rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    default:
        return;
    }
    
    switch(Paint.Mirror) {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X > Paint.WidthMemory || Y > Paint.HeightMemory){
        ESP_LOGI(TAG,"Exceeding display boundaries");
        return;
    }
    
    if(Paint.Scale == 2){
        UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
        UBYTE Rdata = Paint.Image[Addr];
        if(Color == BLACK)
            Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
        else
            Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
    }else if(Paint.Scale == 4){
        UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;//Guaranteed color scale is 4  --- 0~3
        UBYTE Rdata = Paint.Image[Addr];
        Rdata = Rdata & (~(0xC0 >> ((X % 4)*2)));//Clear first, then set value
        Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4)*2));
    }else if(Paint.Scale == 6 || Paint.Scale == 7 || Paint.Scale == 16){
		UDOUBLE Addr = X / 2  + Y * Paint.WidthByte;
		UBYTE Rdata = Paint.Image[Addr];
		Rdata = Rdata & (~(0xF0 >> ((X % 2)*4)));//Clear first, then set value
		Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2)*4));
		// printf("Add =  %d ,data = %d\r\n",Addr,Rdata);
	}
}

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void Paint_Clear(UWORD Color)
{	
	if(Paint.Scale == 2) {
		for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
			for (UWORD X = 0; X < Paint.WidthByte; X++ ) {//8 pixel =  1 byte
				UDOUBLE Addr = X + Y*Paint.WidthByte;
				Paint.Image[Addr] = Color;
			}
		}		
    }else if(Paint.Scale == 4) {
        for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
			for (UWORD X = 0; X < Paint.WidthByte; X++ ) {
				UDOUBLE Addr = X + Y*Paint.WidthByte;
				Paint.Image[Addr] = (Color<<6)|(Color<<4)|(Color<<2)|Color;
			}
		}		
	}else if(Paint.Scale == 6 || Paint.Scale == 7 || Paint.Scale == 16) {
		for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
			for (UWORD X = 0; X < Paint.WidthByte; X++ ) {
				UDOUBLE Addr = X + Y*Paint.WidthByte;
				Paint.Image[Addr] = (Color<<4)|Color;
			}
		}		
	}
}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/
void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color)
{
    UWORD X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) {//8 pixel =  1 byte
            Paint_SetPixel(X, Y, Color);
        }
    }
}

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		: The Xpoint coordinate of the point
    Ypoint		: The Ypoint coordinate of the point
    Color		: Painted color
    Dot_Pixel	: point size
    Dot_Style	: point Style
******************************************************************************/
void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color,
                     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style)
{
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        ESP_LOGI(TAG,"Paint_DrawPoint Input exceeds the normal display range");
        return;
    }

    int16_t XDir_Num , YDir_Num;
    if (Dot_Style == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if(Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                // printf("x = %d, y = %d\r\n", Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel);
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num <  Dot_Pixel; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num <  Dot_Pixel; YDir_Num++) {
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
            }
        }
    }
}

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
    Line_width : Line width
    Line_Style: Solid and dotted lines
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        ESP_LOGI(TAG,"Paint_DrawLine Input exceeds the normal display range");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        //Painted dotted line, 2 point is really virtual
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Line_width, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the rectangle
******************************************************************************/
void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,
                         UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        ESP_LOGI(TAG,"Input exceeds the normal display range");
        return;
    }

    if (Draw_Fill) {
        UWORD Ypoint;
        for(Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color , Line_width, LINE_STYLE_SOLID);
        }
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color, Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color, Line_width, LINE_STYLE_SOLID);
    }
}

/******************************************************************************
function: Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  ：Center X coordinate
    Y_Center  ：Center Y coordinate
    Radius    ：circle Radius
    Color     ：The color of the ：circle segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the Circle
******************************************************************************/
void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius,
                      UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
{
    if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
        ESP_LOGI(TAG,"Paint_DrawCircle Input exceeds the normal display range");
        return;
    }

    //Draw a circle from(0, R) as a starting point
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = Radius;

    //Cumulative error,judge the next point of the logo
    int16_t Esp = 3 - (Radius << 1 );

    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent ) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//1
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//2
                Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//3
                Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//4
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//5
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//6
                Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//7
                Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent ) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//1
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//2
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//3
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//4
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//5
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//6
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//7
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//0

            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
function: Show English characters
parameter:
    Xpoint           ：X coordinate
    Ypoint           ：Y coordinate
    Acsii_Char       ：To display the English characters
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawChar(UWORD Xstart, UWORD Ystart, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Page, Column;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        ESP_LOGI(TAG,"Paint_DrawChar Input exceeds the normal display range");
        return;
    }

    if ((unsigned char)Acsii_Char < 0x20) {
        ESP_LOGW(TAG, "Unsupported ASCII control characters: 0x%02X", (unsigned char)Acsii_Char);
        return;
    }

    uint32_t Char_Offset = ((unsigned char)Acsii_Char - 0x20) * Font->size;
    unsigned char font_buffer[Font->size];

    FILE *file = fopen(Font->font_name, "rb");
    if (!file) {
        ESP_LOGE(TAG, "The font file cannot be opened: %s", Font->font_name);
        return;
    }
    fseek(file, Char_Offset, SEEK_SET);
    size_t read_size = fread(font_buffer, 1, Font->size, file);
    fclose(file);

    if (read_size != Font->size) {
        ESP_LOGE(TAG, "Failed to read the font data: %c", Acsii_Char);
        return;
    }

    const unsigned char *ptr = font_buffer;
    for (Page = 0; Page < Font->Height; Page++) {
        for (Column = 0; Column < Font->Width; Column++) {
            if (FONT_BACKGROUND == Color_Background) {
                if (*ptr & (0x80 >> (Column % 8)))
                    Paint_SetPixel(Xstart + Column, Ystart + Page, Color_Foreground);
            } else {
                if (*ptr & (0x80 >> (Column % 8))) {
                    Paint_SetPixel(Xstart + Column, Ystart + Page, Color_Foreground);
                } else {
                    Paint_SetPixel(Xstart + Column, Ystart + Page, Color_Background);
                }
            }
            if (Column % 8 == 7)
                ptr++;
        }
        if (Font->Width % 8 != 0)
            ptr++;
    }
}

/******************************************************************************
function:	Display the string
parameter:
    Xstart           ：X coordinate
    Ystart           ：Y coordinate
    pString          ：The first address of the English string to be displayed
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{
    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        ESP_LOGI(TAG,"Paint_DrawString_EN Input exceeds the normal display range");
        return;
    }

    while (* pString != '\0') {
        //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
        if ((Xpoint + Font->Width ) > Paint.Width ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > Paint.Height ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        Paint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Foreground, Color_Background);

        //The next character of the address
        pString ++;

        //The next word of the abscissa increases the font of the broadband
        Xpoint += Font->Width;
    }
}


/******************************************************************************
function: Display the string
parameter:
    Xstart  ：X coordinate
    Ystart  ：Y coordinate
    pString ：The first address of the Chinese string and English
              string to be displayed
    Font    ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString, cFONT* font,UWORD Color_Foreground, UWORD Color_Background)
{
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j;
    unsigned char font_buffer[700];

    // ESP_LOGI(TAG, "Start drawing Chinese strings: %s", pString);

    while (*p_text != '\0') {
        int char_len = 1;
        
        if (font->encoding == FONT_ENCODING_UTF8) {
            char_len = Get_UTF8_Char_Length((unsigned char)*p_text);
        } else {
            char_len = (*p_text < 0x80) ? 1 : 2;
        }
        
        char current_char[5] = {0};
        strncpy(current_char, p_text, char_len);
        
        int font_data_size = Get_Char_Font_Data(font, current_char, font_buffer);
        if (font_data_size <= 0) {
            ESP_LOGW(TAG, "The character font data cannot be read: %s", current_char);
            p_text += char_len;
            continue;
        }
        
        int char_width = (char_len == 1) ? font->Width_EN : font->Width_CH;
        int char_height = font->Height;
        
        if (x + char_width > Paint.Width) {
            x = Xstart;
            y += char_height;
        }
        
        if (y + char_height > Paint.Height) {
            ESP_LOGW(TAG, "The string is beyond the display area");
            break;
        }
        
        const unsigned char* ptr = font_buffer;
        for (j = 0; j < char_height; j++) {
            for (i = 0; i < char_width; i++) {
                if (FONT_BACKGROUND == Color_Background) {
                    if (*ptr & (0x80 >> (i % 8))) {
                        Paint_SetPixel(x + i, y + j, Color_Foreground);
                    }
                } else {
                    if (*ptr & (0x80 >> (i % 8))) {
                        Paint_SetPixel(x + i, y + j, Color_Foreground);
                    } else {
                        Paint_SetPixel(x + i, y + j, Color_Background);
                    }
                }
                
                if (i % 8 == 7) {
                    ptr++;
                }
            }
            if (char_width % 8 != 0) {
                ptr++;
            }
        }
        
        // Move to the next character position
        p_text += char_len;
        x += char_width;
        
        // ESP_LOGD(TAG, "Draw character :% s, position :(%d,%d), width :%d", current_char, x - char_width, y, char_width);
    }
    // ESP_LOGI(TAG, "The Chinese string drawing has been completed");
}

/******************************************************************************
function:	Display nummber
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
#define  ARRAY_LEN 255
void Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Foreground, UWORD Color_Background)
{

    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
    uint8_t *pStr = Str_Array;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        ESP_LOGI(TAG,"Paint_DisNum Input exceeds the normal display range");
        return;
    }

    //Converts a number to a string
    do {
        Num_Array[Num_Bit] = Nummber % 10 + '0';
        Num_Bit++;
        Nummber /= 10;
    } while(Nummber);
    

    //The string is inverted
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit ++;
        Num_Bit --;
    }

    //show
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Foreground, Color_Background);
}

/******************************************************************************
function:	Display nummber (Able to display decimals)
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             ：A structure pointer that displays a character size
    Digit            : Fractional width
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawNumDecimals(UWORD Xpoint, UWORD Ypoint, double Nummber,
                    sFONT* Font, UWORD Digit, UWORD Color_Foreground, UWORD Color_Background)
{
    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
    uint8_t *pStr = Str_Array;
	int temp = Nummber;
	float decimals;
	uint8_t i;
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        ESP_LOGI(TAG,"Paint_DisNum Input exceeds the normal display range");
        return;
    }

	if(Digit > 0) {		
		decimals = Nummber - temp;
		for(i=Digit; i > 0; i--) {
			decimals*=10;
		}
		temp = decimals;
		//Converts a number to a string
		for(i=Digit; i>0; i--) {
			Num_Array[Num_Bit] = temp % 10 + '0';
			Num_Bit++;
			temp /= 10;						
		}	
		Num_Array[Num_Bit] = '.';
		Num_Bit++;
	}

	temp = Nummber;
    //Converts a number to a string
    do {
        Num_Array[Num_Bit] = temp % 10 + '0';
        Num_Bit++;
        temp /= 10;
    } while(temp);

    //The string is inverted
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit ++;
        Num_Bit --;
    }

    //show
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Foreground, Color_Background);
}

/******************************************************************************
function:	Display time
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    pTime            : Time-related structures
    Font             ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font,
                    UWORD Color_Foreground, UWORD Color_Background)
{
    uint8_t value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    UWORD Dx = Font->Width;

    //Write data into the cache
    Paint_DrawChar(Xstart                           , Ystart, value[pTime->Hour / 10], Font, Color_Foreground, Color_Background);
    Paint_DrawChar(Xstart + Dx                      , Ystart, value[pTime->Hour % 10], Font, Color_Foreground, Color_Background);
    Paint_DrawChar(Xstart + Dx  + Dx / 4 + Dx / 2   , Ystart, ':'                    , Font, Color_Foreground, Color_Background);
    Paint_DrawChar(Xstart + Dx * 2 + Dx / 2         , Ystart, value[pTime->Min / 10] , Font, Color_Foreground, Color_Background);
    Paint_DrawChar(Xstart + Dx * 3 + Dx / 2         , Ystart, value[pTime->Min % 10] , Font, Color_Foreground, Color_Background);
    Paint_DrawChar(Xstart + Dx * 4 + Dx / 2 - Dx / 4, Ystart, ':'                    , Font, Color_Foreground, Color_Background);
    Paint_DrawChar(Xstart + Dx * 5                  , Ystart, value[pTime->Sec / 10] , Font, Color_Foreground, Color_Background);
    Paint_DrawChar(Xstart + Dx * 6                  , Ystart, value[pTime->Sec % 10] , Font, Color_Foreground, Color_Background);
}

/******************************************************************************
function:	Display monochrome bitmap
parameter:
    image_buffer ：A picture data converted to a bitmap
info:
    Use a computer to convert the image into a corresponding array,
    and then embed the array directly into Imagedata.cpp as a .c file.
******************************************************************************/
void Paint_DrawBitMap(const unsigned char* image_buffer)
{
    UWORD x, y;
    UDOUBLE Addr = 0;

    for (y = 0; y < Paint.HeightByte; y++) {
        for (x = 0; x < Paint.WidthByte; x++) {//8 pixel =  1 byte
            Addr = x + y * Paint.WidthByte;
            Paint.Image[Addr] = (unsigned char)image_buffer[Addr];
        }
    }
}




// 图像旋转90度（顺时针），src/dst均为4bit packed格式
// src: 原始图像缓存（不含flag），dst: 目标缓存（不含flag），w/h为原始宽高
void Paint_RotateImage90_4bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int new_w = h, new_h = w;
    int dst_pixels = new_w * new_h;
    int dst_bytes = (dst_pixels + 1) / 2;
    memset(dst, 0xFF, dst_bytes);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // 原图(x, y) → 新图(y, w - x - 1)
            int src_idx = y * w + x;
            int src_byte = src_idx / 2;
            int src_nibble = src_idx % 2;
            uint8_t src_val = (src[src_byte] >> (4 * (1 - src_nibble))) & 0x0F;

            int dst_x = y;
            int dst_y = w - x - 1;
            int dst_idx = dst_y * new_w + dst_x;
            int dst_byte = dst_idx / 2;
            int dst_nibble = dst_idx % 2;
            if (dst_nibble == 0)
                dst[dst_byte] = (dst[dst_byte] & 0x0F) | (src_val << 4);
            else
                dst[dst_byte] = (dst[dst_byte] & 0xF0) | src_val;
        }
    }
}


// 顺时针旋转180度，src/dst均为4bit packed格式
void Paint_RotateImage180_4bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int pixels = w * h;
    int dst_bytes = (pixels + 1) / 2;
    memset(dst, 0xFF, dst_bytes);

    for (int i = 0; i < pixels; ++i) {
        int src_byte = i / 2;
        int src_nibble = i % 2;
        uint8_t src_val = (src[src_byte] >> (4 * (1 - src_nibble))) & 0x0F;

        int j = pixels - 1 - i; // 180度旋转后的位置
        int dst_byte = j / 2;
        int dst_nibble = j % 2;
        if (dst_nibble == 0)
            dst[dst_byte] = (dst[dst_byte] & 0x0F) | (src_val << 4);
        else
            dst[dst_byte] = (dst[dst_byte] & 0xF0) | src_val;
    }
}
// 顺时针旋转270度，src/dst均为4bit packed格式
void Paint_RotateImage270_4bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int new_w = h, new_h = w;
    int dst_bytes = (new_w * new_h + 1) / 2;
    memset(dst, 0xFF, dst_bytes);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int src_idx = y * w + x;
            int src_byte = src_idx / 2;
            int src_nibble = src_idx % 2;
            uint8_t src_val = (src[src_byte] >> (4 * (1 - src_nibble))) & 0x0F;

            // 270度旋转后坐标
            int nx = h - 1 - y;
            int ny = x;
            int dst_idx = ny * new_w + nx;
            int dst_byte = dst_idx / 2;
            int dst_nibble = dst_idx % 2;
            if (dst_nibble == 0)
                dst[dst_byte] = (dst[dst_byte] & 0x0F) | (src_val << 4);
            else
                dst[dst_byte] = (dst[dst_byte] & 0xF0) | src_val;
        }
    }
}
// 辅助函数：获取/设置4bit像素
uint8_t get_pixel_4bit(const uint8_t* buf, int x, int y, int width)
{
    int idx = y * width + x;
    int byte = idx / 2;
    int nibble = idx % 2;
    return (buf[byte] >> (4 * (1 - nibble))) & 0x0F;
}
void set_pixel_4bit(uint8_t* buf, int x, int y, int width, uint8_t val)
{
    int idx = y * width + x;
    int byte = idx / 2;
    int nibble = idx % 2;
    if (nibble == 0)
        buf[byte] = (buf[byte] & 0x0F) | (val << 4);
    else
        buf[byte] = (buf[byte] & 0xF0) | (val & 0x0F);
}

// 分块旋转，节省内存，适合4bit/pixel格式
// src_dst: 原缓存区（旋转后也写回这里），w/h为原始宽高
void Paint_RotateImage90_4bit_inplace(uint8_t* src_dst, int w, int h)
{
    // 只适合正方形图像，否则会覆盖
    if (w != h) {
        // 非正方形建议用双buffer
        return;
    }
    int N = w; // N x N
    // 每行有N像素，4bit/pixel
    for (int layer = 0; layer < N / 2; ++layer) {
        int first = layer;
        int last = N - 1 - layer;
        for (int i = first; i < last; ++i) {
            int offset = i - first;
            // 四个点交换
            uint8_t p1 = get_pixel_4bit(src_dst, first, i, N);
            uint8_t p2 = get_pixel_4bit(src_dst, last - offset, first, N);
            uint8_t p3 = get_pixel_4bit(src_dst, last, last - offset, N);
            uint8_t p4 = get_pixel_4bit(src_dst, i, last, N);

            set_pixel_4bit(src_dst, first, i, N, p2);
            set_pixel_4bit(src_dst, last - offset, first, N, p3);
            set_pixel_4bit(src_dst, last, last - offset, N, p4);
            set_pixel_4bit(src_dst, i, last, N, p1);
        }
    }
}
// 4bit/pixel正方形原地180度旋转
void Paint_RotateImage180_4bit_inplace(uint8_t* buf, int N)
{
    int pixels = N * N;
    for (int i = 0; i < pixels / 2; ++i) {
        int j = pixels - 1 - i;
        uint8_t vi = get_pixel_4bit(buf, i % N, i / N, N);
        uint8_t vj = get_pixel_4bit(buf, j % N, j / N, N);
        set_pixel_4bit(buf, i % N, i / N, N, vj);
        set_pixel_4bit(buf, j % N, j / N, N, vi);
    }
}

// 4bit/pixel正方形原地270度旋转
void Paint_RotateImage270_4bit_inplace(uint8_t* buf, int N)
{
    // 270度等价于逆时针90度
    // 采用四点交换法
    for (int layer = 0; layer < N / 2; ++layer) {
        int first = layer;
        int last = N - 1 - layer;
        for (int i = first; i < last; ++i) {
            int offset = i - first;
            uint8_t p1 = get_pixel_4bit(buf, first, i, N);
            uint8_t p2 = get_pixel_4bit(buf, i, last, N);
            uint8_t p3 = get_pixel_4bit(buf, last, last - offset, N);
            uint8_t p4 = get_pixel_4bit(buf, last - offset, first, N);

            set_pixel_4bit(buf, first, i, N, p2);
            set_pixel_4bit(buf, i, last, N, p3);
            set_pixel_4bit(buf, last, last - offset, N, p4);
            set_pixel_4bit(buf, last - offset, first, N, p1);
        }
    }
}



// src: 原图缓存，dst: 目标缓存，w/h为原图宽高（像素）
// 旋转后宽高互换，dst需分配 h*w/8 字节
void RotateBitmap90_1bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int new_w = h, new_h = w;
    memset(dst, 0, (new_w * new_h + 7) / 8);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int src_idx = y * w + x;
            int src_byte = src_idx / 8;
            int src_bit = 7 - (src_idx % 8);
            uint8_t pixel = (src[src_byte] >> src_bit) & 0x01;

            // 旋转后坐标
            int nx = y;
            int ny = new_h - x - 1;
            int dst_idx = ny * new_w + nx;
            int dst_byte = dst_idx / 8;
            int dst_bit = 7 - (dst_idx % 8);
            if (pixel)
                dst[dst_byte] |= (1 << dst_bit);
            else
                dst[dst_byte] &= ~(1 << dst_bit);
        }
    }
}
// src: 原图缓存，dst: 目标缓存，w/h为原图宽高（像素）
// 旋转后宽高不变
void RotateBitmap180_1bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int pixels = w * h;
    int dst_bytes = (pixels + 7) / 8;
    memset(dst, 0, dst_bytes);

    for (int i = 0; i < pixels; ++i) {
        int src_byte = i / 8;
        int src_bit = 7 - (i % 8);
        uint8_t pixel = (src[src_byte] >> src_bit) & 0x01;

        int j = pixels - 1 - i; // 180度旋转后的位置
        int dst_byte = j / 8;
        int dst_bit = 7 - (j % 8);
        if (pixel)
            dst[dst_byte] |= (1 << dst_bit);
        else
            dst[dst_byte] &= ~(1 << dst_bit);
    }
}
// src: 原图缓存，dst: 目标缓存，w/h为原图宽高（像素）
// 旋转后宽高互换
void RotateBitmap270_1bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int new_w = h, new_h = w;
    memset(dst, 0, (new_w * new_h + 7) / 8);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int src_idx = y * w + x;
            int src_byte = src_idx / 8;
            int src_bit = 7 - (src_idx % 8);
            uint8_t pixel = (src[src_byte] >> src_bit) & 0x01;

            // 270度旋转后坐标
            int nx = h - 1 - y;
            int ny = x;
            int dst_idx = ny * new_w + nx;
            int dst_byte = dst_idx / 8;
            int dst_bit = 7 - (dst_idx % 8);
            if (pixel)
                dst[dst_byte] |= (1 << dst_bit);
            else
                dst[dst_byte] &= ~(1 << dst_bit);
        }
    }
}


// src: 原图缓存，dst: 目标缓存，w/h为原图宽高（像素）
// 旋转后宽高互换，dst需分配 h*w/4 字节
void RotateBitmap90_2bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int new_w = h, new_h = w;
    memset(dst, 0, (new_w * new_h + 3) / 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int src_idx = y * w + x;
            int src_byte = src_idx / 4;
            int src_shift = 6 - 2 * (src_idx % 4);
            uint8_t pixel = (src[src_byte] >> src_shift) & 0x03;

            // 旋转后坐标
            int nx = y;
            int ny = new_h - x - 1;
            int dst_idx = ny * new_w + nx;
            int dst_byte = dst_idx / 4;
            int dst_shift = 6 - 2 * (dst_idx % 4);
            dst[dst_byte] &= ~(0x03 << dst_shift);
            dst[dst_byte] |= (pixel & 0x03) << dst_shift;
        }
    }
}
// 顺时针180度旋转，src/dst均为2bit packed格式
void RotateBitmap180_2bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int pixels = w * h;
    int dst_bytes = (pixels + 3) / 4;
    memset(dst, 0, dst_bytes);

    for (int i = 0; i < pixels; ++i) {
        int src_byte = i / 4;
        int src_shift = 6 - 2 * (i % 4);
        uint8_t pixel = (src[src_byte] >> src_shift) & 0x03;

        int j = pixels - 1 - i;
        int dst_byte = j / 4;
        int dst_shift = 6 - 2 * (j % 4);
        dst[dst_byte] &= ~(0x03 << dst_shift);
        dst[dst_byte] |= (pixel & 0x03) << dst_shift;
    }
}

// 顺时针270度旋转，src/dst均为2bit packed格式
void RotateBitmap270_2bit(const uint8_t* src, uint8_t* dst, int w, int h)
{
    int new_w = h, new_h = w;
    memset(dst, 0, (new_w * new_h + 3) / 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int src_idx = y * w + x;
            int src_byte = src_idx / 4;
            int src_shift = 6 - 2 * (src_idx % 4);
            uint8_t pixel = (src[src_byte] >> src_shift) & 0x03;

            // 270度旋转后坐标
            int nx = h - 1 - y;
            int ny = x;
            int dst_idx = ny * new_w + nx;
            int dst_byte = dst_idx / 4;
            int dst_shift = 6 - 2 * (dst_idx % 4);
            dst[dst_byte] &= ~(0x03 << dst_shift);
            dst[dst_byte] |= (pixel & 0x03) << dst_shift;
        }
    }
}











