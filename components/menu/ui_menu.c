/*
   This code generates an effect that should pass the 'fancy graphics' qualification
   as set in the comment in the spi_master code.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_system.h>
#include <math.h>
#include "decode_image.h"
#include "ui_menu.h"
#include "driver/gpio.h"
#include "charPixels.c"

#include <display.h>
#include <gamepad.h>

uint16_t **pixels;
int yOff;
int newX;
int bootTV;
int whiteN;
int slow;
int page;
int test;
int change;
int choosen;
int lineMax;
int selRom;

bool peGetPixel(char peChar, int pe1, int pe2){
	return cpGetPixel(peChar,pe1,pe2);
}

void setLineMax(int lineM){
	lineMax = lineM;
}

void setSelRom(int selR){
	selRom=selR;
}

int getSelRom(){
	return selRom;
}
//!!! Colors repeat after 3Bit(example: 001 = light green, 111 = max green -> 1000 = again light green),
//		 so all values over (dec) 7 start to repeat the color, but they are stored in 5bits!!!
//returns a 16bit rgb Color (1Bit + 15Bit bgr), values for each Color from 0-31
//(MSB=? + 5Bits blue + 5Bits red + 5Bits green)
int rgbColor(int red, int green, int blue){
	return 0x8000+blue*1024+red*32+green;
}

int getNoise(){
	whiteN=rand()%8;
	return rgbColor(whiteN,whiteN,whiteN);
}

//Grab a rgb16 pixel from the esp32_tiles image, scroll part of it in
static inline uint16_t bootScreen(int x, int y, int yOff, int bootTV){
	if(gpio_get_level(START)==1)test=0;
	if(bootTV<slow*251 && bootTV>slow*150) return 0;//getNoise();
	else if(bootTV>0){
		if(x>125 && x<142 && y>105 && y<114){
			int xAct = (x/2);
			int yAct = (y/2);
			if(pixels[yAct+40][xAct]<0x8000+1000)return 0;//0x8000+31;
			else return 0;//getNoise();
		}
		return 0;//getNoise();
	}
	if(x>=0 && x <=160){
	if(y<80 && pixels[y][x]!=0x0000 ){
		return pixels[y][x];
	}
	else y=y-yOff/8;

	if(y<80 || pixels[y][x]==0x0000){
		return 0;//getNoise();
	}

    return pixels[y][x];}
	else return getNoise();
}

//run "boot screen" (intro) and later menu to choose a rom
static inline uint16_t get_bgnd_pixel(int x, int y, int yOff, int bootTV, int choosen1)
{
	page=0;
	if(test>=0){
	    if(y==0)test--;
	    return bootScreen(x,y,yOff,bootTV);
	}

	return getCharPixel(x, y, change, choosen1);
}


//This variable is used to detect the next frame.
static int prev_frame=-1;
input_gamepad_state previous_state;

void ui_menu_calc_lines(uint16_t *dest, int line, int frame, int linect)
{
	input_gamepad_state state;
	gamepad_read(&state);

	if(bootTV>0)bootTV--;
    if(yOff>0 && bootTV==0)yOff--;

	if(previous_state.values[GAMEPAD_INPUT_UP] && !state.values[GAMEPAD_INPUT_UP] && choosen>0){
		choosen-=1;
	}
	if(previous_state.values[GAMEPAD_INPUT_DOWN] && !state.values[GAMEPAD_INPUT_DOWN] && choosen<lineMax){
		choosen+=1;
	}
	if(previous_state.values[GAMEPAD_INPUT_A] && !state.values[GAMEPAD_INPUT_A]) {
		selRom=choosen;
	}
	if (frame!=prev_frame) {
		//variable for blinking icons - very ugly solution, i know
		change+=1;
		if(change == 30)change = 0;
        prev_frame=frame;
    }

    for (int y=line; y<line+linect; y++) {
		for (int x=0; x<160; x++){
			*dest++=get_bgnd_pixel(x, y, yOff, bootTV, choosen);
        }
    }
	previous_state = state;
}

void freeMem(){
	for (int i=0; i<128; i++) {
            free((pixels)[i]);
        }
    free(pixels);
	freeRL();
}

//initialize varibles for "timers" and input, gpios and load picture
esp_err_t ui_menu_init()
{
    slow=4;
    yOff=slow*880;
	bootTV=slow*250;
	test=slow*6000;
	choosen=0;
	lineMax = 0;
	initRomList();
	return decode_image(&pixels);
}
