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

#define DEFAULT_FRAME_WIDTH     256
#define DEFAULT_FRAME_HEIGHT    240

#define U16x2toU32(m,l)         ((((uint32_t)(l>>8|(l&0xFF)<<8))<<16)|(m>>8|(m&0xFF)<<8))
#define AVERAGE(a, b)           ( ((((a) ^ (b)) & 0xf7deU) >> 1) + ((a) & (b)) )

#define LINE_BUFFERS            (2)
#define LINE_COUNT              (1)

uint16_t* line[LINE_BUFFERS];
extern uint16_t myPalette[];

void send_lines(int ypos, uint16_t *linedata)
{
   #if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
        st7735r_send_lines(ypos, linedata);
   #endif

   #if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
        ili9341_send_lines(ypos, linedata);
   #endif
}

/*
void send_line_finish()
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x=0; x<6; x++) {
        ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret==ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}
*/

static uint16_t averageSamples(const uint8_t * data[], int dx, int dy)
{
	uint16_t a,b;
	a = AVERAGE(myPalette[(unsigned char) (data[dy*DEFAULT_FRAME_HEIGHT/LCD_HEIGHT][dx*DEFAULT_FRAME_WIDTH/LCD_WIDTH])],myPalette[(unsigned char) (data[dy*DEFAULT_FRAME_HEIGHT/LCD_HEIGHT][(dx*DEFAULT_FRAME_WIDTH/LCD_WIDTH) + 1])]);
	b = AVERAGE(myPalette[(unsigned char) (data[(dy*DEFAULT_FRAME_HEIGHT/LCD_HEIGHT) + 1][(dx*DEFAULT_FRAME_WIDTH/LCD_WIDTH)])],myPalette[(unsigned char) (data[(dy*DEFAULT_FRAME_HEIGHT/LCD_HEIGHT) + 1][(dx*DEFAULT_FRAME_WIDTH/LCD_WIDTH) + 1])]);
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
                a = 0;
                b = 0;
            }
            else
            {
	            a = averageSamples(data, x, y);
		        b = averageSamples(data, x, y);
            }
		    line[calc_line][x]=U16x2toU32(a,b);
		}
		//if (sending_line!=-1) send_line_finish();
		sending_line=calc_line;
		calc_line=(calc_line==1)?0:1;
		send_lines(y, line[sending_line]);
	}
    //send_line_finish();
}

int display_menu()
{
    int frame=0;
    int sending_line=-1;
    int calc_line=0;
    while(1) {
		frame++;
        for (int y=0; y<128; y++) {
            //Calculate a line.
            ui_menu_calc_lines(line[calc_line], y, frame, 1);
            //if (sending_line!=-1) send_line_finish();
            sending_line=calc_line;
            calc_line=(calc_line==1)?0:1;
            send_lines(y, line[sending_line]);
			if(getSelRom()!=12345){
                //send_line_finish();
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
    //Initialize the LCD
    disp_spi_init();
#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ST)
    st7735r_init();
#endif

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI)
    ili9341_init();
#endif
}
