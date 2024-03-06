#ifndef BSP_AUDIO_H
#define BSP_AUDIO_H

#include "esp_err.h"

#define GPIO_I2S_LRCK (GPIO_NUM_20)
#define GPIO_I2S_MCLK (GPIO_NUM_NC)
#define GPIO_I2S_SCLK (GPIO_NUM_2)
#define GPIO_I2S_SDIN (GPIO_NUM_21)
#define GPIO_I2S_DOUT (GPIO_NUM_NC)

#define MAX_FILE_SIZE (256000)
#define BIT_PER_SAMPLE (16)  //只验证了16位采样
#define NUM_CHANNELS        (1) // 只验证了单声道，双声道未验证
#define SAMPLE_RATE (16000)
#define SAMPLE_SIZE         (BIT_PER_SAMPLE * 1024)
#define BYTE_RATE           (SAMPLE_RATE * (BIT_PER_SAMPLE / 8)) * NUM_CHANNELS

void bsp_i2s_init(uint32_t sample_rate);
esp_err_t bsp_get_feed_data(void *buffer, size_t bytes_read, size_t *bytes_written);

#endif
