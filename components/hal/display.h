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

#define PIN_NUM_MOSI    CONFIG_HW_LCD_MOSI_GPIO
#define PIN_NUM_CLK     CONFIG_HW_LCD_CLK_GPIO
#define PIN_NUM_CS      CONFIG_HW_LCD_CS_GPIO
#define PIN_NUM_DC      CONFIG_HW_LCD_DC_GPIO
#define PIN_NUM_RST     CONFIG_HW_LCD_RESET_GPIO
#define PIN_NUM_BCKL    CONFIG_HW_LCD_BL_GPIO

#define MADCTL_MY       0x80
#define MADCTL_MX       0x40
#define MADCTL_MV       0x20
#define MADCTL_ML       0x10
#define MADCTL_RGB      0x00
#define MADCTL_BGR      0x08
#define MADCTL_MH       0x04

#define LCD_WIDTH       160
#define LCD_HEIGHT      128

void display_init();
void send_lines(int ypos, uint16_t *linedata);
void send_line_finish();
void write_nes_frame(const uint8_t * data[]);
int display_menu();

#ifdef __cplusplus
}
#endif

#endif //  _DISPLAY_H_
