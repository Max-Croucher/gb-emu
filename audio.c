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
#include "cpu.h"
#include "audio.h"

#define DEVICE_FORMAT           ma_format_f32
#define DEVICE_CHANNELS         2
#define DEVICE_SAMPLE_RATE      48000
#define GAMEBOY_CHANNELS        4
#define BASE_CAPACITANCE        0.999958
#define CAPACITANCE             (pow(BASE_CAPACITANCE, ((float)(CLK_HZ))/((float)(DEVICE_SAMPLE_RATE))))
#define AUDIO_BUF_NUM_FRAMES    (1800*3)
#define AUDIO_BUF_NUM_SAMPLES   (AUDIO_BUF_NUM_FRAMES * DEVICE_CHANNELS)

extern uint8_t* ram;
extern uint16_t system_counter;
static uint8_t div_apu = 1;
static bool last_div_bit = 0;
static uint16_t gb_sample_index = 0;
static double capacitors[DEVICE_CHANNELS] = {0.0};

typedef struct channel_attributes {
    uint8_t sample_state;
    uint8_t length_timer;
    uint8_t envelope_timer;
    uint8_t amplitude;
    uint8_t duty_period;
    uint16_t pulse_period;
}channel_attributes;

static channel_attributes channels[GAMEBOY_CHANNELS];

ma_waveform ch2_waveform;
ma_device device;

static float flip_flop_state = 0.2;
static uint16_t flip_flop_timer = 100;

static uint32_t buf_readpos = 0;
static uint32_t buf_writepos = 0;
static float audio_buffer[AUDIO_BUF_NUM_SAMPLES];


void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    /* callback routine for miniaudio. This function is automatically called when
    the audio engine is running out of samples, to allow the main thread to copy
    at most frameCount frames into the buffer. A frame consists of one sample per
    channel. The gameboy supports two audio channels (left + right), thus a frame
    consists of two 32-bit floats, or 8 bytes. This routine combines samples from
    all four voices and applies the global amplitude and enable flags. */

    float* pFramesOutF32 = (float*)pOutput;
    // for (ma_uint64 frame = 0; frame < frameCount; frame += 1) {
    //     for (ma_uint64 i = 0; i < ((ma_waveform*)pDevice->pUserData)->config.channels; i += 1) {
    //         pFramesOutF32[frame*((ma_waveform*)pDevice->pUserData)->config.channels + i] = flip_flop_state;
    //     }
    //     flip_flop_timer--;
    //     if (!flip_flop_timer) {
    //         flip_flop_state *= -1;
    //         flip_flop_timer = 100;
    //     }
    // }

    ma_uint64 buffered_frames = ((buf_writepos - buf_readpos) / DEVICE_CHANNELS) % AUDIO_BUF_NUM_FRAMES;
    if (frameCount > buffered_frames) frameCount = buffered_frames;

    if (buf_readpos + frameCount * DEVICE_CHANNELS < AUDIO_BUF_NUM_SAMPLES) { // buf does not wrap around. do one memcpy
        memcpy(pFramesOutF32, &audio_buffer[buf_readpos], sizeof(float) * frameCount * DEVICE_CHANNELS);
        buf_readpos += frameCount * DEVICE_CHANNELS;
    } else { // buf wraps around. Do two memcpys
        memcpy(pFramesOutF32, &audio_buffer[buf_readpos], sizeof(float) * (AUDIO_BUF_NUM_SAMPLES-buf_readpos));
        uint32_t new_readpos = (frameCount * DEVICE_CHANNELS - (AUDIO_BUF_NUM_SAMPLES-buf_readpos));
        memcpy(&pFramesOutF32[AUDIO_BUF_NUM_SAMPLES-buf_readpos], audio_buffer, sizeof(float) * new_readpos);
        buf_readpos = new_readpos;
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

    memset(channels, 0, sizeof(channel_attributes) * GAMEBOY_CHANNELS); // init channels attrs to 0

}


void close_audio(void) {
    /* close the audio devices */
    ma_device_uninit(&device);
    ma_waveform_uninit(&ch2_waveform);  /* Uninitialize the waveform after the device so we don't pull it from under the device while it's being reference in the data callback. */
}


static inline void event_length(void) {
    /* process the ticking of the length timer */
    if ((read_byte(0xFF26)&2) && (read_byte(0xFF19)&0x80)) { // is channel 2 on and length enabled?
        channels[1].length_timer++;
        if (channels[1].length_timer >= 64) {
            *(ram+0xFF26) &= ~2; // disable channel 2
        }
    }
}


static inline void event_ch1_freq_sweep(void) {
    /* process the ticking of channel 1's frequency sweep */
}


static inline void event_envelope_sweep(void) {
    /* process the ticking of the amplitude envelope */
}


float high_pass(float value, uint8_t cap_id) {
    /* simulate the high-pass filter capacitor in the gameboy's audio circuit */
    float out = value - capacitors[cap_id];
    capacitors[cap_id] = value - out * CAPACITANCE;
    return out;
}


static inline void write_to_buf(float val) {
    /* write val to the audio buffer and increment write head */
    audio_buffer[buf_writepos++];
    printf("%.4f\n", val);
    if (buf_writepos >= AUDIO_BUF_NUM_SAMPLES) buf_writepos = 0;
}


static inline void queue_sample(void) {
    /* queue the current sample into the buffer to be copied by the miniaudio callback.
    For each enabled DAC, perform the following:
    Translate each channels sample [0, F] to an analog signal [1, -1]
    Combine each analog sample into a left and right channel, as determined by NR51
    Scale left and right channels by NR50
    Apply High-Pass Filter
    Add to buffer
    */
    float samples[DEVICE_CHANNELS] = {0,0};
    // for (uint8_t i=0; i<GAMEBOY_CHANNELS; i++) { // FIX ME FOR DEAD CHANNELS - only add channels if DAC is on
    //     float curr_sample = ((float)channels[i].sample_state / 30.0) - 0.25; // scale to [-0.25, 0.25]
    //     if (read_byte(0xFF25) & 1 << i    ) samples[0] += curr_sample; // right
    //     if (read_byte(0xFF25) & 1 << (i+4)) samples[1] += curr_sample; // left
    // }
    
    // temp for ch2 only
    float curr_sample = ((float)channels[1].sample_state / 7.5) - 1.0; // scale to [-1, 1]
    if (read_byte(0xFF25) & 2 ) samples[0] += curr_sample; // right
    if (read_byte(0xFF25) & 32) samples[1] += curr_sample; // left

    // at this point, each device channel's sample is in the range [-1,1]

    for (uint8_t i=0; i<DEVICE_CHANNELS; i++) {
        printf(" %.4f ", samples[i]);
        if (read_byte(0xFF24) & (1 << (3 + (4*i)))){ // is master channel on
            samples[i] = 0;
            printf(" (%c off) ", i ? 'r' : 'l');

        } else {
            samples[i] *= (float)(((read_byte(0xFF24) >> (4*i)) & 7) + 1) / 8.0; // scale by master volume
            printf(" %.4f ", samples[i]);
            samples[i] = high_pass(samples[i], i);
            printf(" (%c on)  ", i ? 'r' : 'l');
        }
        write_to_buf(samples[i]);

    }
}


void handle_audio_register(uint16_t addr) {
    /* handle writing to an audio register. */
    switch (addr)
    {
    case 0xFF19: //NR24

        if (read_byte(0xFF17) & 0xF8) { // only enable channel if DAC is on
            *(ram+0xFF26) |= 2; // enable channel 2
            if (channels[1].length_timer >= 64) channels[1].length_timer = read_byte(0xFF16)&0x63; // reset length timer if expired
            channels[1].pulse_period = *(ram+0xFF18) + ((uint16_t)(*(ram+0xFF19)&7)<<8); // set pulse period
            channels[1].amplitude = read_byte(0xFF17) >> 4; // set sweep amplitude
            channels[1].duty_period = 0; // reset phase of pulse
        }
        break;
    
    default:
        break;
    }
}


static inline void tick_channel_1(void) {
    /* process the ticking of channel 1 (pulse + sweep) */
}


static inline void tick_channel_2(void) {
    /* process the ticking of channel 2 (pulse). This function is called at a rate of 1MHz */
    if (read_byte(0xFF26) & 2) { // is channel 2 on?
        channels[1].pulse_period++;
        if (channels[1].pulse_period >= 0x7FF) { // overflow at 2047
            channels[1].pulse_period = *(ram+0xFF18) + ((uint16_t)(*(ram+0xFF19)&7)<<8); // reset to the channel 2 period

            uint8_t duty_limit = (read_byte(0xFF16)>>6) * 2;
            if (!duty_limit) duty_limit++;

            if ((channels[1].duty_period%8) < duty_limit) { // sample is low
                channels[1].sample_state = 0;
                fprintf(stdout, "<");
            } else { // sample is high
                channels[1].sample_state = channels[1].amplitude;
                fprintf(stdout, ">");
            }
            channels[1].duty_period++;
        }
    }
}


static inline void tick_channel_3(void) {
    /* process the ticking of channel 3 (wave) */
}


static inline void tick_channel_4(void) {
    /* process the ticking of channel 4 (noise) */
}


inline void tick_audio(void) {
    /* tick every t-cycle and handle timing of all audio events */
    bool div_bit = (system_counter>>12)&1; // bit 4 of div
    if (last_div_bit && !div_bit) {
        div_apu++;

        if (div_apu % 2 == 0) event_length();
        if (div_apu % 4 == 0) event_ch1_freq_sweep();
        if (div_apu == 8) {
            event_envelope_sweep();
            div_apu = 0;
        }

    }
    if (system_counter % 4 == 0) {
        tick_channel_1();
        tick_channel_2();
    }
    // tick_channel_3();
    // tick_channel_4();

    // send 375 samples to audio buf every 32,768 gameboy ticks
    if (gb_sample_index % 175 == 0 || gb_sample_index % 175 == 87) queue_sample();
    gb_sample_index++;
    last_div_bit = div_bit;
}