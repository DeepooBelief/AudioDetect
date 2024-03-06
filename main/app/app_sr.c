#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "string.h"
#include "bsp_audio.h"
#include "app_sr.h"
#include "app_audio.h"

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static volatile int task_flag = 0;
QueueHandle_t result_que;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int feed_channel = 1;
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag)
    {
        bsp_get_feed_data(i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel, NULL);

        afe_handle->feed(afe_data, i2s_buff);
        audio_record_save(i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

    }
    if (i2s_buff)
    {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    static afe_vad_state_t local_state;
    static uint8_t frame_keep = 0;

    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------detect start------------\n");

    while (task_flag)
    {
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL)
        {
            printf("fetch error!\n");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED)
        {
            printf("wakeword detected\n");
            sr_result_t result = {
                .wakenet_mode = WAKENET_DETECTED,
                .state = ESP_MN_STATE_DETECTING,
                .command_id = 0,
            };
            xQueueSend(result_que, &result, 0);
            printf("-----------LISTENING-----------\n");
        }else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED)
        {
            detect_flag = 1;
            // if ((word_detect_callback) != NULL) (*word_detect_callback)();
            printf("AFE_FETCH_CHANNEL_VERIFIED, channel index: %d\n", res->trigger_channel_id);
            afe_handle->disable_wakenet(afe_data);
        }
        if (detect_flag)
        {
            if (local_state != res->vad_state)
            {
                local_state = res->vad_state;
                frame_keep = 0;
            }
            else
            {
                frame_keep++;
            }
            if ((50 == frame_keep) && (AFE_VAD_SILENCE == res->vad_state))
            {
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = ESP_MN_STATE_TIMEOUT,
                    .command_id = 0,
                };
                printf("timeout\n");
                xQueueSend(result_que, &result, 0);
                // if (wd_detect_callback_timeout != NULL) (*wd_detect_callback_timeout)();
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }
        }
    }
    if (buff)
    {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

void model_init(void)
{
    // 模型初始化
    srmodel_list_t *models = esp_srmodel_init("model");
    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    ;
    afe_config.aec_init = false;
    afe_config.pcm_config.total_ch_num = 1;
    afe_config.pcm_config.mic_num = 1;
    afe_config.pcm_config.ref_num = 0;
    afe_data = afe_handle->create_from_config(&afe_config);
}

void app_audio_detect_init(void)
{
    result_que = xQueueCreate(3, sizeof(sr_result_t));
    model_init();
    audio_record_init();
    task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void *)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&sr_handler_task, "SR Handler Task", 8 * 1024, NULL, 5, NULL, 0);
}

esp_err_t app_sr_get_result(sr_result_t *result, TickType_t xTicksToWait)
{
    xQueueReceive(result_que, result, xTicksToWait);
    return ESP_OK;
}