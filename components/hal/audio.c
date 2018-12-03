#include "audio.h"

void audio_init(int sample_rate)
{
    printf("%s: sample_rate=%d\n", __func__, sample_rate);

    // NOTE: buffer needs to be adjusted per AUDIO_SAMPLE_RATE
    i2s_config_t i2s_config = {
        //.mode = I2S_MODE_MASTER | I2S_MODE_TX,  // Only TX
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .sample_rate = sample_rate,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 2-channels
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        //.dma_buf_len = 1472 / 2,  // (368samples * 2ch * 2(short)) = 1472
        .dma_buf_len = 512,  // (416samples * 2ch * 2(short)) = 1664
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, //Interrupt level 1
        .use_apll = 0 //1
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);

    i2s_set_pin(I2S_NUM, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);

    //I2S enables *both* DAC channels; we only need DAC1.
	//ToDo: still needed now I2S supports set_dac_mode?
	CLEAR_PERI_REG_MASK(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC_XPD_FORCE_M);
	CLEAR_PERI_REG_MASK(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC_M);
}

void audio_submit(short* stereoAudioBuffer, int frameCount)
{
    short currentAudioSampleCount = frameCount * 2;

    // Convert for built in DAC
    for (short i = 0; i < currentAudioSampleCount; i += 2)
    {
         uint16_t dac0;
         uint16_t dac1;

         // Down mix stero to mono
         int32_t sample = stereoAudioBuffer[i];
         sample += stereoAudioBuffer[i + 1];
         sample >>= 1;
         // Normalize
         const float sn = (float)sample / 0x8000;
         // Scale
         const int magnitude = 127 + 127;
         const float range = magnitude  * sn;

         // Convert to differential output
         if (range > 127)
         {
             dac1 = (range - 127);
             dac0 = 127;
         }
         else if (range < -127)
         {
             dac1  = (range + 127);
             dac0 = -127;
         }
         else
         {
             dac1 = 0;
             dac0 = range;
         }
         dac0 += 0x80;
         dac1 = 0x80 - dac1;

         dac0 <<= 8;
         dac1 <<= 8;

         stereoAudioBuffer[i] = (int16_t)dac1;
         stereoAudioBuffer[i + 1] = (int16_t)dac0;
    }

    int len = currentAudioSampleCount * sizeof(int16_t);
    int count = i2s_write_bytes(I2S_NUM, (const char *)stereoAudioBuffer, len, portMAX_DELAY);
    if (count != len)
    {
        printf("i2s_write_bytes: count (%d) != len (%d)\n", count, len);
        abort();
    }
}
