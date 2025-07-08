/* Source file for audio.c, emulating gameboy audio
    Author: Max Croucher
    Email: mpccroucher@gmail.com
    June 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "miniaudio.h"
#include "miniaudio.c"
#include "audio.h"

#define DEVICE_FORMAT       ma_format_f32
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000

extern uint8_t* ram;
extern uint16_t system_counter;
uint8_t div_apu = 1;
bool last_sysclk_bit = 0;

ma_waveform ch2_waveform;
ma_device device;

static float flip_flop_state = 0.2;
static uint16_t flip_flop_timer = 100;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    /* callback routine for miniaudio. This function is automatically called when
    the audio engine is running out of samples, to allow the main thread to copy
    at most frameCount frames into the buffer. A frame consists of one sample per
    channel. The gameboy supports two audio channels (left + right), thus a frame
    consists of two 32-bit floats, or 8 bytes. This routine combines samples from
    all four voices and applies the global amplitude and enable flags. */

    float* pFramesOutF32 = (float*)pOutput;
    for (ma_uint64 frame = 0; frame < frameCount; frame += 1) {
        for (ma_uint64 i = 0; i < ((ma_waveform*)pDevice->pUserData)->config.channels; i += 1) {
            pFramesOutF32[frame*((ma_waveform*)pDevice->pUserData)->config.channels + i] = flip_flop_state;
        }
        flip_flop_timer--;
        if (!flip_flop_timer) {
            flip_flop_state *= -1;
            flip_flop_timer = 100;
        }
    }
}


void init_audio(void) {
    /* initialise the audio controller */
    ma_device_config device_config;
    ma_waveform_config ch2_waveform_config;

    device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = DEVICE_FORMAT;
    device_config.playback.channels = DEVICE_CHANNELS;
    device_config.sampleRate        = DEVICE_SAMPLE_RATE;
    device_config.dataCallback      = data_callback;
    device_config.pUserData         = &ch2_waveform;

    if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        exit(EXIT_FAILURE);
    }
    printf("Loaded audio device %s\n", device.playback.name);
    ch2_waveform_config = ma_waveform_config_init(device.playback.format, device.playback.channels, device.sampleRate, ma_waveform_type_square, 0.2, 240);
    ma_waveform_init(&ch2_waveform_config, &ch2_waveform);

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        exit(EXIT_FAILURE);
    }
}


void close_audio(void) {
    /* close the audio devices */
    ma_device_uninit(&device);
    ma_waveform_uninit(&ch2_waveform);  /* Uninitialize the waveform after the device so we don't pull it from under the device while it's being reference in the data callback. */
}


void event_length(void) {
    /* process the ticking of the length timer */
}


void event_ch1_freq_sweep(void) {
    /* process the ticking of channel 1's frequency sweep */
}


void event_envelope_sweep(void) {
    /* process the ticking of the amplitude envelope */
}


void tick_audio(void) {
    /* tick every t-cycle and handle timing of all audio events */
    bool sysclk_bit = (system_counter>>4)&1;
    if (last_sysclk_bit && !sysclk_bit) {
        div_apu++;

        if (div_apu % 2 == 0) event_length();
        if (div_apu % 4 == 0) {event_ch1_freq_sweep();}
        if (div_apu == 8) {
            event_envelope_sweep();
            div_apu = 0;
        }

    }
    last_sysclk_bit = sysclk_bit;
}