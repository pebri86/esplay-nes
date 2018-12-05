#include "ui_menu.h"
#include <display.h>

int runMenu()
{
    //Initialize the menu displayed
    ui_menu_init();
	setSelRom(12345);
	return display_menu();
}
