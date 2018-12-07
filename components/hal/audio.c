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
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
}

void audio_submit(uint8_t *buf, int len)//short* stereoAudioBuffer, int frameCount)
{
   /* short currentAudioSampleCount = frameCount * 2;

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
    int len = currentAudioSampleCount * sizeof(int16_t);

        for (short i = 0; i < currentAudioSampleCount; ++i)
        {
            int sample = stereoAudioBuffer[i];

            if (sample > 32767)
                sample = 32767;
            else if (sample < -32768)
                sample = -32767;

            stereoAudioBuffer[i] = (short)sample;
        }

    int count = i2s_write_bytes(I2S_NUM, (const char *)stereoAudioBuffer, len, portMAX_DELAY);
    if (count != len)
    {
        printf("i2s_write_bytes: count (%d) != len (%d)\n", count, len);
        abort();
    }*/
    uint32_t tmpb[32];
	int i=0;
	while (i<len) {
		int plen=len-i;
		if (plen>32) plen=32;
		for (int j=0; j<plen; j++) {
			int s=((((int)buf[i+j])-128)*4); //Make [-128,127], multiply with volume
			s=(s>>8)+(4/2); //divide off volume max, get back to [0-maxvol]
			if (s>255) s=255;
			if (s<0) s=0;
			tmpb[j]=((s)<<8)+((s)<<24);
		}
		i2s_write_bytes(0, (char*)tmpb, plen*4, portMAX_DELAY);
		i+=plen;
	}
}
