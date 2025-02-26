#include "led.h"
#include "pico/stdlib.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "ws2812.pio.h"

#define WS2812_PIN 16

PIO pixelPio;
uint pixelSm;
uint8_t pixelBuffer[3];

uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void led_put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pixelPio, pixelSm, pixel_grb << 8u);
}

void led_init()
{
    pixelSm = 0;
    pixelPio = pio0;

    uint offset = pio_add_program(pixelPio, &ws2812_program);
    ws2812_program_init(pixelPio, pixelSm, offset, WS2812_PIN, 800000, false);

    led_put_pixel(0);
    sleep_ms(1);
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    led_put_pixel(urgb_u32(r, g, b));
}
