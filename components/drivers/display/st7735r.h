/**
 * @file st7735r.h
 *
 */

#ifndef ST7735R_H
#define ST7735R_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
//#include "../lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define ST7735R_HOR_RES	160
#define ST7735R_VER_RES	128

#define ST7735R_DC   CONFIG_HW_LCD_DC_GPIO
#define ST7735R_RST  CONFIG_HW_LCD_RESET_GPIO
#define ST7735R_BCKL CONFIG_HW_LCD_BL_GPIO

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void st7735r_init(void);
void st7735r_send_lines(int ypos, uint16_t *linedata);
//void st7735r_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color);
//void st7735r_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_map);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ST7735R_H*/
