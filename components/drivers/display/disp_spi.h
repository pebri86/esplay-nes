/**
 * @file lv_templ.h
 *
 */

#ifndef LV_TEMPL_H
#define LV_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

#define DISP_SPI_MOSI CONFIG_HW_LCD_MOSI_GPIO
#define DISP_SPI_CLK  CONFIG_HW_LCD_CLK_GPIO
#define DISP_SPI_CS   CONFIG_HW_LCD_CS_GPIO


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void disp_spi_init(void);
void disp_spi_send(uint8_t * data, uint16_t length);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_TEMPL_H*/
