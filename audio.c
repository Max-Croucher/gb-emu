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
static uint16_t audio_sample_divider = CLK_HZ / DEVICE_SAMPLE_RATE;

static uint16_t REG_NRx1[4] = {REG_NR11,REG_NR21,REG_NR31,REG_NR41};
static uint16_t REG_NRx2[4] = {REG_NR12,REG_NR22,REG_NR32,REG_NR42};
static uint16_t REG_NRx3[4] = {REG_NR13,REG_NR23,REG_NR33,REG_NR43};
static uint16_t REG_NRx4[4] = {REG_NR14,REG_NR24,REG_NR34,REG_NR44};

typedef struct channel_attributes {
    uint8_t sample_state;
    uint8_t length_timer;
    uint8_t amp_sweep_timer;
    uint8_t amp_sweep_attrs;
    uint8_t amplitude;
    uint8_t duty_period;
    uint16_t pulse_period;

}channel_attributes;

static channel_attributes channels[GAMEBOY_CHANNELS];

ma_waveform master_waveform;
ma_device device;

 uint8_t ch1_freq_sweep_timer = 0;;


static uint32_t buf_readpos = 0;
static uint32_t buf_writepos = 0;
static float audio_buffer[AUDIO_BUF_NUM_SAMPLES];

//FILE* raw_audio_file;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    /* callback routine for miniaudio. This function is automatically called when
    the audio engine is running out of samples, to allow the main thread to copy
    at most frameCount frames into the buffer. A frame consists of one sample per
    channel. The gameboy supports two audio channels (left + right), thus a frame
    consists of two 32-bit floats, or 8 bytes. This routine combines samples from
    all four voices and applies the global amplitude and enable flags. */

    float* pFramesOutF32 = (float*)pOutput;

    ma_uint64 buffered_frames = ((AUDIO_BUF_NUM_SAMPLES + buf_writepos - buf_readpos) % AUDIO_BUF_NUM_SAMPLES) / 2;
    //printf("Reqesting %d frames. Divider is %d. Reader is at %d, Writer is at %d. %lld are available.", frameCount, audio_sample_divider, buf_readpos, buf_writepos, buffered_frames);
    if (frameCount > buffered_frames) frameCount = buffered_frames;
    //printf(" Copying %d frames from position %d", frameCount, buf_readpos);

    if (buffered_frames) {
        if (buffered_frames > 1200) {
            audio_sample_divider=88;
        } else {
            audio_sample_divider=87;
        }
    }

    if (buf_readpos + frameCount * DEVICE_CHANNELS < AUDIO_BUF_NUM_SAMPLES) { // buf does not wrap around. do one memcpy
        //printf(" (contiguous)\n");
        //fwrite(&audio_buffer[buf_readpos], sizeof(float), frameCount * DEVICE_CHANNELS, raw_audio_file);
        memcpy(pFramesOutF32, &audio_buffer[buf_readpos], sizeof(float) * frameCount * DEVICE_CHANNELS);
        buf_readpos += frameCount * DEVICE_CHANNELS;
    } else { // buf wraps around. Do two memcpys
        //printf(" (wrapped): %ld bytes at end", sizeof(float) * (AUDIO_BUF_NUM_SAMPLES-buf_readpos));
        //fwrite(&audio_buffer[buf_readpos], sizeof(float), AUDIO_BUF_NUM_SAMPLES-buf_readpos, raw_audio_file);
        memcpy(pFramesOutF32, &audio_buffer[buf_readpos], sizeof(float) * (AUDIO_BUF_NUM_SAMPLES-buf_readpos));
        uint32_t new_readpos = (frameCount * DEVICE_CHANNELS - (AUDIO_BUF_NUM_SAMPLES-buf_readpos));
        //printf(" and %ld bytes at start\n", sizeof(float) * new_readpos);
        //fwrite(audio_buffer, sizeof(float), new_readpos, raw_audio_file);
        memcpy(&pFramesOutF32[AUDIO_BUF_NUM_SAMPLES-buf_readpos], audio_buffer, sizeof(float) * new_readpos);
        buf_readpos = new_readpos;
    }
}


void init_audio(void) {
    /* initialise the audio controller */
    ma_device_config device_config;
    ma_waveform_config master_waveform_config;

    //raw_audio_file = fopen("raw_audio.waveform", "wb");

    device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = DEVICE_FORMAT;
    device_config.playback.channels = DEVICE_CHANNELS;
    device_config.sampleRate        = DEVICE_SAMPLE_RATE;
    device_config.dataCallback      = data_callback;
    device_config.pUserData         = &master_waveform;

    if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        exit(EXIT_FAILURE);
    }
    printf("Loaded audio device %s\n", device.playback.name);
    master_waveform_config = ma_waveform_config_init(device.playback.format, device.playback.channels, device.sampleRate, ma_waveform_type_square, 0.2, 240);
    ma_waveform_init(&master_waveform_config, &master_waveform);

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
    ma_waveform_uninit(&master_waveform);  /* Uninitialize the waveform after the device so we don't pull it from under the device while it's being reference in the data callback. */
}


static inline void event_length(void) {
    /* process the ticking of the length timer */
    for (uint8_t i=0; i<2; i++) {
        if ((read_byte(REG_NR52)&(1<<i)) && (read_byte(REG_NRx4[i])&0x40)) { // is channel i on and length enabled?
            channels[i].length_timer++;
            if (channels[i].length_timer >= 64) {
                *(ram+REG_NR52) &= ~(1<<i); // disable channel i
            }
        }
    }
}


static inline void event_ch1_freq_sweep(void) {
    /* process the ticking of channel 1's frequency sweep */
    uint8_t step = *(ram+REG_NR10)&7;
    bool direction = *(ram+REG_NR10)&8;
    uint8_t pace = (*(ram+REG_NR10)>>4)&7;

    uint16_t delta = channels[0].pulse_period / (uint16_t)pow(2, step);

    if (direction && channels[0].pulse_period + delta > 0x7FF) { // pending overflow; immediately disable
        *(ram+REG_NR52) &= ~1; // disable channel 1
        return;
    }

    // otherwise, proceed as normal
    if ((read_byte(REG_NR10)>>4)&7) { // only tick if pace is not 0
        ch1_freq_sweep_timer--;
        if (ch1_freq_sweep_timer == 0) { // change freq
            if (direction) {
                channels[0].pulse_period += delta;
            } else {
                channels[0].pulse_period -= delta;
            }
            *(ram+REG_NR13) = channels[0].pulse_period & 0xFF;
            *(ram+REG_NR13) &= 0xF8;
            *(ram+REG_NR13) |= channels[0].pulse_period>>8;
        }
    }
}


static inline void event_envelope_sweep(void) {
    /* process the ticking of the amplitude envelope */
    for (uint8_t i=0; i<2; i++) {
        if ((read_byte(REG_NR52)&(1<<i)) && (read_byte(REG_NRx2[i])&7)) { // is channel i on and sweep not 0 ?
            channels[i].amp_sweep_timer++;
            if (channels[i].amp_sweep_timer >= channels[i].amp_sweep_attrs&7) { // sweep pace
                channels[i].amp_sweep_timer = 0;
                if (channels[i].amp_sweep_attrs&8) { // env dir
                    if (channels[i].amplitude != 15) channels[i].amplitude++;
                } else {
                    if (channels[i].amplitude != 0) channels[i].amplitude--;
                }
            }
        }
    }
}


float high_pass(float value, uint8_t cap_id) {
    /* simulate the high-pass filter capacitor in the gameboy's audio circuit */
    float out = value - capacitors[cap_id];
    capacitors[cap_id] = value - out * CAPACITANCE;
    return out;
}


static inline void write_to_buf(float val) {
    /* write val to the audio buffer and increment write head */
    audio_buffer[buf_writepos++] = val;
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
    bool dac_state[4] = {
        read_byte(REG_NR12) & 0xF8,
        read_byte(REG_NR22) & 0xF8,
        read_byte(REG_NR30) & 128,
        read_byte(REG_NR42) & 0xF8
    };
    for (uint8_t i=0; i<GAMEBOY_CHANNELS; i++) {
        if (dac_state[i]) {
            float curr_sample = ((float)channels[i].sample_state / 30.0) - 0.25; // scale to [-0.25, 0.25]
            if (read_byte(REG_NR51) & 1 << i    ) samples[0] += curr_sample; // right
            if (read_byte(REG_NR51) & 1 << (i+4)) samples[1] += curr_sample; // left
            }
    }

    // at this point, each device channel's sample is in the range [-1,1]

    for (uint8_t i=0; i<DEVICE_CHANNELS; i++) {
        //printf(" %.4f ", samples[i]);
        if (read_byte(REG_NR50) & (1 << (3 + (4*i)))){ // is master channel on
            samples[i] = 0;
            //printf(" (%c off) ", i ? 'r' : 'l');

        } else {
            samples[i] *= (float)(((read_byte(REG_NR50) >> (4*i)) & 7) + 1) / 8.0; // scale by master volume
            //printf(" %.4f ", samples[i]);
            samples[i] = high_pass(samples[i], i);
            //printf(" (%c on)  ", i ? 'r' : 'l');
        }
        write_to_buf(samples[i]);

    }
}


void enable_pulse_channel(bool channel_id) {
    /* start a pulse channel */
    if (read_byte(REG_NRx2[channel_id]) & 0xF8) { // only enable channel if DAC is on
        *(ram+REG_NR52) |= (1<<channel_id); // enable channel
        if (channels[channel_id].length_timer >= 64) channels[channel_id].length_timer = read_byte(REG_NRx1[channel_id])&0x63; // reset length timer if expired
        channels[channel_id].pulse_period = *(ram+REG_NRx3[channel_id]) + (((uint16_t)(*(ram+REG_NRx4[channel_id])&7))<<8); // set pulse period
        channels[channel_id].amplitude = read_byte(REG_NRx2[channel_id]) >> 4; // set sweep amplitude
        channels[channel_id].duty_period = 0; // reset phase of pulse
        channels[channel_id].amp_sweep_timer = 0; // reset amplitude envelope timer
        channels[channel_id].amp_sweep_attrs = read_byte(REG_NRx2[channel_id]) & 15; // store amplitude sweep pace and direction
    }
}


void handle_audio_register(uint16_t addr) {
    /* handle writing to an audio register. */
    switch (addr)
    {
    case REG_NR14: // channel 1
        enable_pulse_channel(0);
        ch1_freq_sweep_timer = (read_byte(REG_NR10)>>4)&7;
        break;
    case REG_NR24: // channel 2
        enable_pulse_channel(1);
        break;
    
    default:
        break;
    }
}


static inline void tick_pulse_channel(bool channel_id) {
    /* process the ticking of channel 1 or 2 (pulse). This function is called at a rate of 1MHz */
    if (read_byte(REG_NR52) & (1<<channel_id)) { // is channel on?
        channels[channel_id].pulse_period++;
        if (channels[channel_id].pulse_period >= 0x7FF) { // overflow at 2047
            channels[channel_id].pulse_period = *(ram+REG_NRx3[channel_id]) + (((uint16_t)(*(ram+REG_NRx4[channel_id])&7))<<8); // reset to the channel period

            uint8_t duty_limit = (read_byte(REG_NRx1[channel_id])>>6) * 2;
            if (!duty_limit) duty_limit++;

            if ((channels[channel_id].duty_period%8) < duty_limit) { // sample is low
                channels[channel_id].sample_state = 0;
                //fprintf(stdout, "<");
            } else { // sample is high
                channels[channel_id].sample_state = channels[channel_id].amplitude;
                //fprintf(stdout, ">");
            }
            channels[channel_id].duty_period++;
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

        if (div_apu % 2 == 0) event_length();           // 256 Hz
        if (div_apu % 4 == 3) event_ch1_freq_sweep();   // 128 Hz
        if (div_apu % 8 == 7) event_envelope_sweep();   // 64 Hz
        div_apu++;
    }
    if (system_counter % 4 == 0) {
        tick_pulse_channel(0);
        tick_pulse_channel(1);
    }
    // tick_channel_3();
    // tick_channel_4();


    // must send 48000 samples every 4194304 ticks (one second)
    // therefore, send 375 samples every 32,768 gameboy ticks

    // if ((gb_sample_index%0x8000) % 175 == 0 || (gb_sample_index%0x8000) % 175 == 87) queue_sample();
    // gb_sample_index++;

    if (gb_sample_index % audio_sample_divider == 0) queue_sample();
    gb_sample_index++;

    last_div_bit = div_bit;
}