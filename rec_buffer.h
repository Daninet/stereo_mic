#ifndef REC_BUFFER_H
#define REC_BUFFER_H

#include <stdint.h>

void rec_init();
int16_t* rec_take(uint8_t mute, uint8_t volume);

#endif
