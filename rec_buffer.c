#include "rec_buffer.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "i2s.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static __attribute__((aligned(8))) pio_i2s i2s;

#define SAMPLE_RATE 48000
#define USB_AUDIO_BUFFER_LEN (2 * SAMPLE_RATE / 1000)
#define USB_AUDIO_BUFFERS 4

static int32_t audio_buffer_items = 0;
static int32_t audio_buffer_write_index = 0;

int audio_data_ready()
{
    return audio_buffer_items > 0;
}

static __attribute__((aligned(8))) uint8_t zero_buffer[USB_AUDIO_BUFFER_LEN * 3];
static __attribute__((aligned(8))) int32_t audio_buffers[USB_AUDIO_BUFFERS][USB_AUDIO_BUFFER_LEN];

static i2s_config my_i2s_config = { 48000, 256, 32, 10, 6, 7, 8, false };

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

__attribute__((aligned(8))) uint8_t usb_buffer[USB_AUDIO_BUFFER_LEN * 3];

// static float phase = 0.0f; // persistent phase between calls

uint8_t* rec_take(uint8_t mute, uint8_t volume)
{
    if (audio_buffer_items == 0 || mute) {
        return zero_buffer;
    }

    // memset(usb_buffer, 0x45, USB_AUDIO_BUFFER_LEN * 4);

    // const float freq = 100.0f;
    // const float rate = 48000.0f;
    // const float inc = freq / rate;

    int buffer_pos = (audio_buffer_write_index + USB_AUDIO_BUFFERS - audio_buffer_items) % USB_AUDIO_BUFFERS;
    int32_t* buf = audio_buffers[buffer_pos];

    // float gain = powf((float)volume / 255.0f, 2.2f);

    for (size_t i = 0; i < USB_AUDIO_BUFFER_LEN; i++) {
        // float s = 2.0f * phase - 1.0f;
        // phase += inc;
        // if (phase >= 1.0f)
        //     phase -= 1.0f;

        // int32_t left = (int32_t)(s * 0x7FFFFF * 1.0f);
        // int32_t right = 0x3FFFFF;

        // uint32_t vl = (uint32_t)(left & 0x00FFFFFF);
        // uint32_t vr = (uint32_t)(right & 0x00FFFFFF);

        // size_t idxL = (i) * 3; /* left sample byte base */
        // size_t idxR = (i + 1) * 3; /* right sample byte base */

        // usb_buffer[idxL + 0] = (uint8_t)(vl & 0xFF);
        // usb_buffer[idxL + 1] = (uint8_t)((vl >> 8) & 0xFF);
        // usb_buffer[idxL + 2] = (uint8_t)((vl >> 16) & 0xFF);

        // usb_buffer[idxR + 0] = (uint8_t)(vr & 0xFF);
        // usb_buffer[idxR + 1] = (uint8_t)((vr >> 8) & 0xFF);
        // usb_buffer[idxR + 2] = (uint8_t)((vr >> 16) & 0xFF);

        int32_t v = (buf[i] >> 8); // * gain
        // v = v * gain;

        usb_buffer[i * 3 + 0] = (uint8_t)(v & 0xFF);
        usb_buffer[i * 3 + 1] = (uint8_t)((v >> 8) & 0xFF);
        usb_buffer[i * 3 + 2] = (uint8_t)((v >> 16) & 0xFF);
    }

    audio_buffer_items--;

    return usb_buffer;
}
