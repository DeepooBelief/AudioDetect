#include <stdio.h>
#include <sys/stat.h>
#include "bsp_sdmmc.h"
#include "bsp_audio.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "OpenAI.h"
#include "esp_log.h"
#include "app_sr.h"
#include "app_audio.h"
#include "app_wifi.h"


#define TAG "main"
#define openai_key "sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

void start_openai(uint8_t* data, size_t length){
    OpenAI_t *openai = OpenAICreate(openai_key);
    OpenAI_AudioTranscription_t *audioTranscription = openai->audioTranscriptionCreate(openai);
    audioTranscription->setResponseFormat(audioTranscription, OPENAI_AUDIO_RESPONSE_FORMAT_JSON);
    audioTranscription->setPrompt(audioTranscription, "请回复简体中文"); // The default will return Traditional Chinese, here we add prompt to make GPT return Simplified Chinese
    audioTranscription->setTemperature(audioTranscription, 0.2);         // float between 0 and 1. Higher value gives more random results.
    audioTranscription->setLanguage(audioTranscription, "zh");           // Set to Chinese to make GPT return faster and more accurate
    // audioTranscription->setResponseFormat(audioTranscription, OPENAI_AUDIO_RESPONSE_FORMAT_JSON);
    // audioTranscription->setTemperature(audioTranscription, 0.2);         // float between 0 and 1. Higher value gives more random results.
    // audioTranscription->setLanguage(audioTranscription, "en");           // Set to Chinese to make GPT return faster and more accurate
    char *text = audioTranscription->file(audioTranscription, data, length, OPENAI_AUDIO_INPUT_FORMAT_WAV);
    ESP_LOGI(TAG, "Text: %s", text);
    openai->audioTranscriptionDelete(audioTranscription);

    OpenAI_ChatCompletion_t *chatCompletion = openai->chatCreate(openai);
    chatCompletion->setModel(chatCompletion, "gpt-3.5-turbo");                 // Model to use for completion. Default is gpt-3.5-turbo
    chatCompletion->setSystem(chatCompletion, "You are ChatGPT, a large language model trained by OpenAI.Knowledge cutoff: 2021-09 Current date: [2024-03]"); // Description of the required assistant
    chatCompletion->setMaxTokens(chatCompletion, 1024);                        // The maximum number of tokens to generate in the completion.
    chatCompletion->setTemperature(chatCompletion, 0.2);                       // float between 0 and 1. Higher value gives more random results.
    chatCompletion->setStop(chatCompletion, "\r");                             // Up to 4 sequences where the API will stop generating further tokens.
    chatCompletion->setPresencePenalty(chatCompletion, 0);                     // float between -2.0 and 2.0. Positive values increase the model's likelihood to talk about new topics.
    chatCompletion->setFrequencyPenalty(chatCompletion, 0);                    // float between -2.0 and 2.0. Positive values decrease the model's likelihood to repeat the same line verbatim.
    chatCompletion->setUser(chatCompletion, "OpenAI-ESP32");                   // A unique identifier representing your end-user, which can help OpenAI to monitor and detect abuse.

    OpenAI_StringResponse_t *result = chatCompletion->message(chatCompletion, text, false);
    if (result->getLen(result) == 1)
    {
        ESP_LOGI(TAG, "Received message. Tokens: %" PRIu32 "", result->getUsage(result));
        char *response = result->getData(result, 0);
        ESP_LOGI(TAG, "%s", response);
    }
    else if (result->getLen(result) > 1)
    {
        ESP_LOGI(TAG, "Received %" PRIu32 " messages. Tokens: %" PRIu32 "", result->getLen(result), result->getUsage(result));
        for (int i = 0; i < result->getLen(result); ++i)
        {
            char *response = result->getData(result, i);
            ESP_LOGI(TAG, "Message[%d]: %s", i, response);
        }
    }
    else if (result->getError(result))
    {
        ESP_LOGE(TAG, "Error! %s", result->getError(result));
    }
    else
    {
        ESP_LOGE(TAG, "Unknown error!");
    }
    result->delete (result);

    free(text);

}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    app_network_start();
    bsp_i2s_init(16000);
    app_audio_detect_init();
}