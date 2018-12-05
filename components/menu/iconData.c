#include <esp_system.h>
int arrow[9][16]  =    {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
						,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
						,{0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0}
						,{0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0}
						,{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
						,{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
						,{0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0}
						,{0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0}
						,{0,31*32+31+0x8000}};

int getIconPixel(char actChar, int xMod, int yMod, int change){
		int colorNumber=0;
		if(actChar==';'){
			colorNumber=arrow[yMod][xMod];
			return arrow[8][colorNumber];
		}
		else return 992;
}
