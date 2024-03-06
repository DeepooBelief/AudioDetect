#ifndef __APP_SR_H
#define __APP_SR_H

#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "freertos/FreeRTOS.h"
typedef struct {
    wakenet_state_t wakenet_mode;
    esp_mn_state_t state;
    int command_id;
} sr_result_t;

void app_audio_detect_init(void);
esp_err_t app_sr_get_result(sr_result_t *result, TickType_t xTicksToWait);

#endif