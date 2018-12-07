#include "ui_menu.h"
#include <display.h>

int runMenu()
{
    esp_err_t ret;
    //Initialize the menu displayed
    ret = ui_menu_init();
    ESP_ERROR_CHECK(ret);
	setSelRom(12345);
	return display_menu();
}
