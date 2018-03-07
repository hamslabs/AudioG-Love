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
#include "driver/i2c.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_adc_cal.h"
#include <math.h>
#include <string.h>


///////////////////////
//	Pin Layout I2S
#define I2S_BCLK		(33)
#define I2S_WS			(26)
#define I2S_DATA_IN 	(25)
#define I2S_DATA_OUT	(23)

///////////////////////
//	Pin Layout ADCs
#define ADC1_IN			(ADC1_CHANNEL_7)	// GPIO_NUM_35
#define ADC2_IN			(ADC2_CHANNEL_0)	// GPIO_NUM_4

///////////////////////
//	Pin Layout I2C
#define I2C0_SCL_PIN	(5)
#define I2C0_SDA_PIN	(18)
#define I2C0_FREQ	(100000)

#define SAMPLE_RATE     (48000)
#define I2S_NUM         (0)

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

i2s_pin_config_t pin_config = {
	.bck_io_num		= I2S_BCLK,
	.ws_io_num		= I2S_WS,
  	.data_out_num	= I2S_DATA_OUT,
   	.data_in_num	= I2S_DATA_IN
 };



void AudioTransportTask(void* params) {
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
			theBuf[i][j][1] = sin(freqLeft * 2 * M_PI * phase / SAMPLE_RATE) * multiplier;
			theBuf[i][j][3] = sin(freqRight * 2 * M_PI * phase / SAMPLE_RATE) * multiplier;
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

void SensorTask(void* params) {
//    esp_err_t status;
//   esp_adc_cal_characteristics_t characteristics;
	int adc1Value;
	int adc2Value;


	// init ADCs
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_IN, ADC_ATTEN_DB_11);
	adc2_config_channel_atten(ADC1_IN, ADC_ATTEN_DB_11);


	// init i2c
	int i2c_master_port = I2C_NUM_0;
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C0_SDA_PIN;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = I2C0_SCL_PIN;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C0_FREQ;
	i2c_param_config(i2c_master_port, &conf);
	i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);


	while(1) {
		adc1Value = adc1_get_raw(ADC1_IN);
		adc2_get_raw(ADC2_IN, ADC_WIDTH_BIT_12, &adc2Value);
		printf("ADC1 raw = %d v = %f pct = %f\n",adc1Value, adc1Value/4096.0*3.3, adc1Value/4096.0);
		printf("ADC2 raw = %d v = %f pct = %f\n",adc2Value, adc2Value/4096.0*3.3, adc2Value/4096.0);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

#define FFTSIZE 64


void app_main() {    
	BaseType_t retVal;

	if ((retVal = xTaskCreatePinnedToCore(AudioTransportTask, "Audioxfer", 1024*3, NULL, 10, NULL, APP_CPU_NUM)) != pdPASS)
	{
		printf("task create failed\n");
	}

#if 1
	if ((retVal = xTaskCreatePinnedToCore(SensorTask, "Sensors", 1024*3, NULL, 5, NULL, APP_CPU_NUM)) != pdPASS)
	{
		printf("task create failed\n");
	}
#endif
	
	while(true)
		vTaskDelay(pdMS_TO_TICKS(1000));
    
}
