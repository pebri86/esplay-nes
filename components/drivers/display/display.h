#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <stdint.h>

//*****************************************************************************
//
// Make sure all of the definitions in this header have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C"
{
#endif

#define LCD_TYPE_ILI    0
#define LCD_TYPE_ST     1

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
#include "ili9341.h"
#define LCD_WIDTH       320
#define LCD_HEIGHT      240
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
#include "st7735r.h"
#define LCD_WIDTH       160
#define LCD_HEIGHT      128
#endif

void display_init();
void send_lines(int ypos, uint16_t *linedata);
//void send_line_finish();
void write_nes_frame(const uint8_t * data[]);
int display_menu();

#ifdef __cplusplus
}
#endif

#endif //  _DISPLAY_H_
