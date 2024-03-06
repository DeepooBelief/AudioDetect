#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "driver/i2s_std.h"
#include "soc/soc_caps.h"
#include "esp_err.h"
#include "esp_log.h"
#include "bsp_sdmmc.h"
#include "bsp_audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "BSP_AUDIO"

static i2s_chan_handle_t rx_handle = NULL; // I2S rx channel handler

void bsp_i2s_init(uint32_t sample_rate)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));
    i2s_std_config_t i2s_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = GPIO_I2S_MCLK,
            .bclk = GPIO_I2S_SCLK,
            .ws = GPIO_I2S_LRCK,
            .dout = GPIO_I2S_DOUT,
            .din = GPIO_I2S_SDIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    i2s_config.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &i2s_config));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
    ESP_LOGI("I2S", "I2S init done");
}

// static int16_t i2s_readraw_buff[SAMPLE_SIZE];
// size_t bytes_read;
// void record_wav(uint32_t rec_time)
// {
//     // Use POSIX and C standard library functions to work with files.
//     int flash_wr_size = 0;
//     ESP_LOGI(TAG, "Opening file");

//     uint32_t flash_rec_time = BYTE_RATE * rec_time;
//     const wav_header_t wav_header =
//         WAV_HEADER_PCM_DEFAULT(flash_rec_time, 16, SAMPLE_RATE, 1);
//     // First check if file exists before creating a new file.
//     struct stat st;
//     if (stat(MOUNT_POINT"/record.wav", &st) == 0) {
//         // Delete it if it exists
//         FILE *f = fopen(MOUNT_POINT"/record.wav", "w");
//         remove(MOUNT_POINT"/record.wav");
//         fclose(f);
//     }

//     // Create new WAV file
//     FILE *f = fopen(MOUNT_POINT"/record.wav", "a");
//     if (f == NULL) {
//         ESP_LOGE(TAG, "Failed to open file for writing");
//         return;
//     }

//     // Write the header to the WAV file
//     fwrite(&wav_header, sizeof(wav_header), 1, f);

//     // Start recording
//     while (flash_wr_size < flash_rec_time) {
//         // Read the RAW samples from the microphone
//         if (i2s_channel_read(rx_handle, (char *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 1000) == ESP_OK) {
//             printf("Bytes read: %d\n", bytes_read);
//             printf("[0] %d [1] %d [2] %d [3]%d ...\n", i2s_readraw_buff[0], i2s_readraw_buff[1], i2s_readraw_buff[2], i2s_readraw_buff[3]);
//             // Write the samples to the WAV file
//             fwrite(i2s_readraw_buff, bytes_read, 1, f);
//             flash_wr_size += bytes_read;
//         } else {
//             printf("Read Failed!\n");
//         }
//     }

//     ESP_LOGI(TAG, "Recording done!");
//     fclose(f);
//     ESP_LOGI(TAG, "File written on SDCard");
// }

esp_err_t bsp_get_feed_data(void *buffer, size_t bytes_read, size_t *bytes_written){
    return i2s_channel_read(rx_handle, buffer, bytes_read, bytes_written, portMAX_DELAY);
}