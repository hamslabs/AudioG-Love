/* I2S Example
    
    This example code will output 100Hz sine wave and triangle wave to 2-channel of I2S driver
    Every 5 seconds, it will change bits_per_sample [16, 24, 32] for i2s data 

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include <math.h>
#include <string.h>


#define SAMPLE_RATE     (48000)
#define I2S_NUM         (0)
#define PI 3.14159265

#define NUMBUFS 28
#define SAMPLES_PER_BUF (64)
#define BUFLEN (SAMPLES_PER_BUF * 2 * sizeof(int32_t))
int16_t theBuf[NUMBUFS][SAMPLES_PER_BUF][4];
int16_t theOtherBuf[SAMPLES_PER_BUF][4];


i2s_config_t i2s_config = {
	.mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,                                  // Only TX
	.sample_rate = SAMPLE_RATE,
	.bits_per_sample = 32,                                              
	.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
	.communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB,
	.dma_buf_count = 6,
	.dma_buf_len = SAMPLES_PER_BUF,
	.use_apll = 0,
	.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
};

#if 0
i2s_pin_config_t pin_config = {
	.bck_io_num = 26,
	.ws_io_num = 25,
  	.data_out_num = 22,
   	.data_in_num = 23
 };
#else
i2s_pin_config_t pin_config = {
	.bck_io_num = 21,
	.ws_io_num = 19,
  	.data_out_num = 18,
   	.data_in_num = 5
 };
#endif

void AudioTransportTask(void* params)
{
	uint32_t phase = 0;
	int readBufNum = 0;
	int writeBufNum = 0;
	float freqLeft  = 48000.0/(SAMPLES_PER_BUF * NUMBUFS) * 11;
	float freqRight = 48000.0/(SAMPLES_PER_BUF * NUMBUFS) * 17;
	float level = 0.333;


	printf("left = %f right = %f\n", freqLeft, freqRight);


	memset(theBuf,0x0000, sizeof(theBuf));

	float multiplier = pow(2,15) * level;
	for (int i=0; i<NUMBUFS; i++) {
		for (int j=0; j<SAMPLES_PER_BUF; j++) {
			theBuf[i][j][1] = sin(freqLeft * 2 * PI * phase / SAMPLE_RATE) * multiplier;
			theBuf[i][j][3] = sin(freqRight * 2 * PI * phase / SAMPLE_RATE) * multiplier;
//			printf("%d\n", theBuf[i][j][0]);
		++phase;
		}
	}

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);

    i2s_read_bytes(I2S_NUM, (char *)&theBuf[readBufNum++], BUFLEN, pdMS_TO_TICKS(50));
    i2s_read_bytes(I2S_NUM, (char *)&theBuf[readBufNum++], BUFLEN, pdMS_TO_TICKS(50));
    i2s_read_bytes(I2S_NUM, (char *)&theBuf[readBufNum++], BUFLEN, pdMS_TO_TICKS(50));
    while (true) {

    	i2s_read_bytes(I2S_NUM, (char *)&theBuf[readBufNum++], BUFLEN, pdMS_TO_TICKS(50));
		if (readBufNum >= NUMBUFS)
			readBufNum = 0;

    	i2s_write_bytes(I2S_NUM, (const char *)&theBuf[writeBufNum++], BUFLEN, pdMS_TO_TICKS(50));
		if (writeBufNum >= NUMBUFS)
			writeBufNum = 0;
    } 
}


void app_main()
{    
	BaseType_t retVal;


	if ((retVal = xTaskCreatePinnedToCore(AudioTransportTask, "Audioxfer", 1024*3, NULL, 5, NULL, APP_CPU_NUM)) != pdPASS)
	{
		printf("task create failed\n");
	}
    
}
