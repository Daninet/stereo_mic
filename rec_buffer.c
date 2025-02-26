#include "rec_buffer.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "i2s.h"
#include <stdlib.h>
#include <string.h>

static __attribute__((aligned(8))) pio_i2s i2s;

#define SAMPLE_RATE 48000
#define USB_AUDIO_BUFFER_LEN (2 * SAMPLE_RATE / 1000)
#define USB_AUDIO_BUFFERS 4

static size_t audio_buffer_items = 0;
static size_t audio_buffer_write_index = 0;

static __attribute__((aligned(8))) int16_t zero_buffer[USB_AUDIO_BUFFER_LEN];
static __attribute__((aligned(8))) int32_t audio_buffers[USB_AUDIO_BUFFERS][USB_AUDIO_BUFFER_LEN];

static i2s_config my_i2s_config = { 48000, 256, 32, 10, 6, 7, 8, false };

static inline int16_t clamp_i16(int32_t sample)
{
    return (int16_t)((sample + (1 << 15)) >> 16);
}

static void process_audio(const int32_t* input, int32_t* output, size_t num_frames)
{
    if (audio_buffer_items < USB_AUDIO_BUFFERS) {
        memcpy(audio_buffers[audio_buffer_write_index], input, num_frames * 2 * sizeof(int32_t));
        audio_buffer_items++;
        audio_buffer_write_index = (audio_buffer_write_index + 1) % USB_AUDIO_BUFFERS;
    }
}

static void dma_i2s_in_handler(void)
{
    /* We're double buffering using chained TCBs. By checking which buffer the
     * DMA is currently reading from, we can identify which buffer it has just
     * finished reading (the completion of which has triggered this interrupt).
     */
    if (*(int32_t**)dma_hw->ch[i2s.dma_ch_in_ctrl].read_addr == i2s.input_buffer) {
        // It is inputting to the second buffer so we can overwrite the first
        process_audio(i2s.input_buffer, i2s.output_buffer, AUDIO_BUFFER_FRAMES);
    } else {
        // It is currently inputting the first buffer, so we write to the second
        process_audio(&i2s.input_buffer[STEREO_BUFFER_SIZE], &i2s.output_buffer[STEREO_BUFFER_SIZE], AUDIO_BUFFER_FRAMES);
    }
    dma_hw->ints0 = 1u << i2s.dma_ch_in_data; // clear the IRQ
}

void rec_init()
{
    memset(zero_buffer, 0, sizeof(zero_buffer));
    i2s_program_start_synched(pio1, &my_i2s_config, dma_i2s_in_handler, &i2s);
}

static float current_gain = 1.0f;

__attribute__((aligned(8))) int16_t usb_buffer[USB_AUDIO_BUFFER_LEN];

int16_t* rec_take(uint8_t mute, uint8_t volume)
{
    if (audio_buffer_items == 0 || mute) {
        return zero_buffer;
    }

    int buffer_pos = (audio_buffer_write_index + USB_AUDIO_BUFFERS - audio_buffer_items) % USB_AUDIO_BUFFERS;
    int32_t* buf = audio_buffers[buffer_pos];

    for (size_t i = 0; i < USB_AUDIO_BUFFER_LEN; i++) {
        // int32_t new_value = buf[i] << 8;
        int32_t new_value = buf[i] * volume;
        usb_buffer[i] = clamp_i16(new_value);
    }

    audio_buffer_items--;

    return usb_buffer;
}
