#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <stdint.h>
#include "esp_err.h"

void ui_menu_calc_lines(uint16_t *dest, int line, int frame, int linect);
esp_err_t ui_menu_init();
bool peGetPixel(char peChar, int pe1, int pe2);
void setLineMax(int lineM);
int getSelRom();
void setSelRom(int selR);
void freeMem();
