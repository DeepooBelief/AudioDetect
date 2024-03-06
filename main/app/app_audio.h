#ifndef __APP_AUDIO_H
#define __APP_AUDIO_H
#include <stdint.h>
#include <stddef.h>

uint8_t* record_wav(uint32_t rec_time, size_t *length);
void audio_record_init(void);
void sr_handler_task(void *pvParam);
void audio_record_save(void *audio_buffer, size_t bytes_num);
#endif
