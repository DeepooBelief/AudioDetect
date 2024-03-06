#include "format_wav.h"
#include <stdlib.h>
#include <string.h>
#include "bsp_audio.h"
#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
//#include "bsp_sdmmc.h"
#include "bsp_audio.h"
#include "esp_heap_caps.h"
#include "app_sr.h"
#include "freertos/FreeRTOS.h"
#include "app_wifi.h"


#define TAG "APP_AUDIO"

static uint8_t *record_audio_buffer = NULL;
bool record_flag = false;
uint32_t record_total_len = 0;
uint32_t file_total_len = 0;
extern void start_openai(uint8_t* data, size_t length);
extern int Cache_WriteBack_Addr(uint32_t addr, uint32_t size);

uint8_t* record_wav(uint32_t rec_time, size_t *length)
{
    ESP_LOGI(TAG, "Start recording...");
    uint32_t total_byte = BYTE_RATE * rec_time;
    *length = total_byte + sizeof(wav_header_t);
    ESP_LOGI(TAG, "Bytes to record: %ld", total_byte);
    if (total_byte + sizeof(wav_header_t) > MAX_FILE_SIZE) {
        ESP_LOGE(TAG, "Error: File size too large");
        return NULL;
    }
    const wav_header_t wav_header =
        WAV_HEADER_PCM_DEFAULT(total_byte, BIT_PER_SAMPLE, SAMPLE_RATE, NUM_CHANNELS);
    // First check if file exists before creating a new file.
    if(record_audio_buffer == NULL) {
        ESP_LOGE(TAG, "Error: record_audio_buffer is NULL, use audio_record_init() to initialize the buffer");
        return NULL; // Return or handle the error condition appropriately
    }

    // Write the header to the WAV file
    memcpy(record_audio_buffer, &wav_header, sizeof(wav_header_t));
    uint8_t *buffer_ptr = record_audio_buffer + sizeof(wav_header_t);
    size_t bytes_read;
    // Start recording
    size_t written_bytes = 0;
    while (written_bytes < total_byte) {
        // Read the RAW samples from the microphone
        if (bsp_get_feed_data(buffer_ptr, SAMPLE_SIZE, &bytes_read) == ESP_OK) {
            // Write the samples to the WAV file
            written_bytes += bytes_read;
            buffer_ptr += bytes_read;
        } else {
            ESP_LOGE(TAG, "Read Failed!");
            break;
        }
    }
    if (written_bytes < total_byte) {
        printf("Recording failed\n");
        return NULL;
    }

    ESP_LOGI(TAG, "Recording done!");
    return record_audio_buffer;
}

void audio_record_init(void)
{
    /* Create file if record to SD card enabled*/
    if(record_audio_buffer != NULL) {
        ESP_LOGE(TAG, "Error: record_audio_buffer is already initialized");
        return; // Return or handle the error condition appropriately
    }
    record_audio_buffer = heap_caps_calloc(1, MAX_FILE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(record_audio_buffer);
    printf("successfully created record_audio_buffer with a size: %zu\n", MAX_FILE_SIZE);

    if (record_audio_buffer == NULL) {
        printf("Error: Failed to allocate memory for buffers\n");
        return; // Return or handle the error condition appropriately
    }
}


static void audio_record_start()
{
    ESP_LOGI(TAG, "### record Start");
    record_flag = true;
    record_total_len = 0;
    file_total_len = sizeof(wav_header_t);
}

void audio_record_save(void *audio_buffer, size_t bytes_num)
{
    if (record_flag) {
        uint8_t *buffer_ptr = record_audio_buffer + sizeof(wav_header_t);
        buffer_ptr += record_total_len;
        if (record_total_len + bytes_num + sizeof(wav_header_t) > MAX_FILE_SIZE) {
            ESP_LOGE(TAG, "Error: File size too large");
            return;
        }
        memcpy(buffer_ptr, audio_buffer, bytes_num);
        record_total_len += bytes_num;
    }
}

static esp_err_t audio_record_stop()
{
    record_flag = false;
    file_total_len += record_total_len;
    ESP_LOGI(TAG, "### record Stop, %" PRIu32 " %" PRIu32 "K", \
             record_total_len, \
             record_total_len / 1024);

    wav_header_t wav_head = WAV_HEADER_PCM_DEFAULT(record_total_len, BIT_PER_SAMPLE, SAMPLE_RATE, NUM_CHANNELS);
    memcpy((void *)record_audio_buffer, &wav_head, sizeof(wav_header_t));
    Cache_WriteBack_Addr((uint32_t)record_audio_buffer, record_total_len);
    return ESP_OK;
}

void sr_handler_task(void *pvParam)
{
    while (true) {
        sr_result_t result = {
            .wakenet_mode = WAKENET_NO_DETECT,
            .state = ESP_MN_STATE_DETECTING,
        };

        app_sr_get_result(&result, pdMS_TO_TICKS(1 * 1000));

        if (ESP_MN_STATE_TIMEOUT == result.state) {
            ESP_LOGI(TAG, "ESP_MN_STATE_TIMEOUT");
            audio_record_stop();
            if(WIFI_STATUS_CONNECTED_OK == wifi_connected_already()) {
                start_openai(record_audio_buffer, record_total_len);
            }
            continue;
        }

        if (WAKENET_DETECTED == result.wakenet_mode) {
            audio_record_start();
            continue;
        }

        if (ESP_MN_STATE_DETECTED & result.state) {
            ESP_LOGI(TAG, "STOP:%d", result.command_id);
            audio_record_stop();
            //How to stop the transmission, when start_openai begins.
            continue;
        }
    }
    vTaskDelete(NULL);
}