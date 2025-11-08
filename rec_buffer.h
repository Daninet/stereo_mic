#ifndef REC_BUFFER_H
#define REC_BUFFER_H

#include <stdint.h>

int audio_data_ready();
void rec_init();
uint8_t* rec_take(uint8_t mute, uint8_t volume);

#endif
