#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "pretty_effect.h"
#include <display.h>

//Simple routine to generate some patterns and send them to the LCD. Don't expect anything too
//impressive. Because the SPI driver handles transactions in the background, we can calculate the next line
//while the previous one is being sent.
int runMenu()
{
    //Initialize the effect displayed
    pretty_effect_init();
    //Go do nice stuff.
	setSelRom(12345);
	return display_menu();
}
