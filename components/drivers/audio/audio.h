#ifndef AUDIO_H
#define AUDIO_H

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "driver/i2s.h"

#define I2S_NUM     I2S_NUM_0

void audio_init(int sample_rate);
void audio_submit(short* stereoAudioBuffer, int frameCount);
#endif
