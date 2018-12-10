#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "display.h"
#include "ui_menu.h"
#include "disp_spi.h"

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
#include "ili9341.h"
#define LCD_WIDTH       ILI9341_HOR_RES
#define LCD_HEIGHT      ILI9341_VER_RES
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
#include "st7735r.h"
#define LCD_WIDTH       ST7735R_HOR_RES
#define LCD_HEIGHT      ST7735R_VER_RES
#endif

#define NES_FRAME_WIDTH 256
#define NES_FRAME_HEIGHT 240

#define U16x2toU32(m,l) ((((uint32_t)(l>>8|(l&0xFF)<<8))<<16)|(m>>8|(m&0xFF)<<8))
#define AVERAGE(a, b) ( ((((a) ^ (b)) & 0xf7deU) >> 1) + ((a) & (b)) )

#define LINE_BUFFERS (2)
#define LINE_COUNT (1)

uint16_t* line[LINE_BUFFERS];
extern uint16_t myPalette[];

static uint16_t averageSamples(const uint8_t * data[], int dx, int dy)
{
	uint16_t a,b;
	int y = dy*NES_FRAME_HEIGHT/LCD_HEIGHT;
	int x = dx*NES_FRAME_WIDTH/LCD_WIDTH;
    a = AVERAGE(myPalette[(unsigned char) (data[y][x])],myPalette[(unsigned char) (data[y][x + 1])]);
    b = AVERAGE(myPalette[(unsigned char) (data[y + 1][x])],myPalette[(unsigned char) (data[y + 1][x + 1])]);
    return AVERAGE(a,b);
}

void write_nes_frame(const uint8_t * data[])
{
    short x,y;
    uint16_t a,b;
	int sending_line=-1;
	int calc_line=0;
    for (y=0; y<LCD_HEIGHT; y++) {
	    for (x=0; x<LCD_WIDTH; x++) {
            if (data == NULL)
            {
                line[calc_line][x] = 0;
            }
            else
            {
	            a = averageSamples(data, x, y);
		        b = averageSamples(data, x, y);
		        line[calc_line][x]=U16x2toU32(a,b);
            }
		}
		if (sending_line!=-1) send_line_finish();
		sending_line=calc_line;
		calc_line=(calc_line==1)?0:1;
		send_lines(y, LCD_WIDTH, line[sending_line]);
	}
    send_line_finish();
}

int display_menu()
{
    int frame=0;
    int sending_line=-1;
    int calc_line=0;
    while(1) {
		frame++;
        for (int y=0; y<LCD_HEIGHT; y++) {
            //Calculate a line.
            ui_menu_calc_lines(line[calc_line], y, frame, 1);
            if (sending_line!=-1) send_line_finish();
            sending_line=calc_line;
            calc_line=(calc_line==1)?0:1;
            send_lines(y, LCD_WIDTH, line[sending_line]);
			if(getSelRom()!=12345){
                send_line_finish();
				freeMem();
				return getSelRom();
			}
		}
    }

	return 0;
}

void display_init()
{
    // Line buffers
    const size_t lineSize = LCD_WIDTH * LINE_COUNT * sizeof(uint16_t);
    for (int x = 0; x < LINE_BUFFERS; x++)
    {
        line[x] = heap_caps_malloc(lineSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!line[x]) abort();
    }
    // Initialize the LCD
    disp_spi_init();
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
    st7735r_init();
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
    ili9341_init();
#endif
}
