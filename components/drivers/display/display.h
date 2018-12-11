#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <stdint.h>

//*****************************************************************************
//
// Make sure all of the definitions in this header have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C"
{
#endif

void display_init();
void write_nes_frame(const uint8_t * data[]);

#ifdef __cplusplus
}
#endif

#endif /*_DISPLAY_H_*/
