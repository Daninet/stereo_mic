#ifndef LED_H
#define LED_H

#include <stdint.h>

void led_init();
void led_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif
