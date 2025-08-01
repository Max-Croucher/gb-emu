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
#include <endian.h>
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
#define AUDIO_BUF_NUM_FRAMES    (1800*4)
#define AUDIO_BUF_NUM_SAMPLES   (AUDIO_BUF_NUM_FRAMES * DEVICE_CHANNELS)
#define ADUIO_SAMPLE_DIVIDER    (CLK_HZ / DEVICE_SAMPLE_RATE)

#define WAV_SAMPLE_RATE         65536
#define WAV_BITS_PER_SAMPLE     8
#define WAV_HEADER_SIZE         44
#define WAV_BYTES_PER_FRAME     ((GAMEBOY_CHANNELS * WAV_BITS_PER_SAMPLE) / 8)
#define WAV_BYTES_PER_SECOND    (WAV_SAMPLE_RATE * WAV_BYTES_PER_FRAME)



extern uint8_t* ram;
extern uint16_t system_counter;
static uint8_t div_apu = 1;
static bool last_div_bit = 0;
static uint16_t gb_sample_index = 0;
static double capacitors[DEVICE_CHANNELS] = {0.0};
static uint16_t audio_sample_divider = ADUIO_SAMPLE_DIVIDER + 1;
bool do_export_wav = 0; //extern
static uint64_t wav_frames_written = 0;

static uint16_t REG_NRx1[4] = {REG_NR11,REG_NR21,REG_NR31,REG_NR41};
static uint16_t REG_NRx2[4] = {REG_NR12,REG_NR22,REG_NR32,REG_NR42};
static uint16_t REG_NRx3[4] = {REG_NR13,REG_NR23,REG_NR33,REG_NR43};
static uint16_t REG_NRx4[4] = {REG_NR14,REG_NR24,REG_NR34,REG_NR44};

static channel_attributes channels[GAMEBOY_CHANNELS];

ma_waveform master_waveform;
ma_device device;

uint8_t ch1_freq_sweep_timer = 0;
uint8_t ch3_buffered_sample = 0;
uint8_t ch3_wave_ram_index = 0;
uint16_t ch4_LFSR = 0;
uint32_t ch4_LFSR_timer = 0;

static uint32_t buf_readpos = 0;
static uint32_t buf_writepos = 0;
static float audio_buffer[AUDIO_BUF_NUM_SAMPLES];

uint64_t frames_written = 0;

FILE* raw_audio_file;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    /* callback routine for miniaudio. This function is automatically called when
    the audio engine is running out of samples, to allow the main thread to copy
    at most frameCount frames into the buffer. A frame consists of one sample per
    channel. The gameboy supports two audio channels (left + right), thus a frame
    consists of two 32-bit floats, or 8 bytes. This routine combines samples from
    all four voices and applies the global amplitude and enable flags. */

    float* pFramesOutF32 = (float*)pOutput;
    frames_written = 0;
    ma_uint64 buffered_frames = ((AUDIO_BUF_NUM_SAMPLES + buf_writepos - buf_readpos) % AUDIO_BUF_NUM_SAMPLES) / 2;
    if (frameCount > buffered_frames) frameCount = buffered_frames;

    if (buffered_frames) {
        if (buffered_frames > (AUDIO_BUF_NUM_FRAMES * 2) / 3) {
            audio_sample_divider = ADUIO_SAMPLE_DIVIDER + 2;
        } else if (buffered_frames > (AUDIO_BUF_NUM_FRAMES) / 3) {
            audio_sample_divider = ADUIO_SAMPLE_DIVIDER + 1; // tick slightly too slow
        } else {
            audio_sample_divider = ADUIO_SAMPLE_DIVIDER; // tick slightly too fast to catch up
        }
    }

    if (buf_readpos + frameCount * DEVICE_CHANNELS < AUDIO_BUF_NUM_SAMPLES) { // buf does not wrap around. do one memcpy
        // fwrite(&audio_buffer[buf_readpos], sizeof(float), frameCount * DEVICE_CHANNELS, raw_audio_file);
        memcpy(pFramesOutF32, &audio_buffer[buf_readpos], sizeof(float) * frameCount * DEVICE_CHANNELS);
        buf_readpos += frameCount * DEVICE_CHANNELS;
    } else { // buf wraps around. Do two memcpys
        // fwrite(&audio_buffer[buf_readpos], sizeof(float), AUDIO_BUF_NUM_SAMPLES-buf_readpos, raw_audio_file);
        memcpy(pFramesOutF32, &audio_buffer[buf_readpos], sizeof(float) * (AUDIO_BUF_NUM_SAMPLES-buf_readpos));
        uint32_t new_readpos = (frameCount * DEVICE_CHANNELS - (AUDIO_BUF_NUM_SAMPLES-buf_readpos));
        // fwrite(audio_buffer, sizeof(float), new_readpos, raw_audio_file);
        memcpy(&pFramesOutF32[AUDIO_BUF_NUM_SAMPLES-buf_readpos], audio_buffer, sizeof(float) * new_readpos);
        buf_readpos = new_readpos;
    }
}


static inline void open_wav_file(void) {
    /* open a wav file to export gameboy audio, and write file header. */
    raw_audio_file = fopen("debug_audio.wav", "wb");
    uint8_t header_format[WAV_HEADER_SIZE] = {
        'R', 'I', 'F', 'F',             // Identifier
        0, 0, 0, 0,                     // File size (minus 8 bytes)
        'W', 'A', 'V', 'E',             // File format

                                        // FORMAT chunk
        'f', 'm', 't', 0x20,            // fmt specifier
        16, 0, 0, 0,                    // Chunk size, 16 bytes (4 bytes little endian)
        1, 0,                           // Audio format (1 is PCM integer, 2 bytes litte endian)
        (GAMEBOY_CHANNELS>>0)&0xFF,     // Num channels (2 bytes little endian)
        (GAMEBOY_CHANNELS>>8)&0xFF,
        (WAV_SAMPLE_RATE>>0)&0xFF,      // Sample Rate (4 bytes little endian)
        (WAV_SAMPLE_RATE>>8)&0xFF,
        (WAV_SAMPLE_RATE>>16)&0xFF,
        (WAV_SAMPLE_RATE>>24)&0xFF,
        (WAV_BYTES_PER_SECOND>>0)&0xFF, // Bytes per second (4 bytes little endian)
        (WAV_BYTES_PER_SECOND>>8)&0xFF,
        (WAV_BYTES_PER_SECOND>>16)&0xFF,
        (WAV_BYTES_PER_SECOND>>24)&0xFF,
        (WAV_BYTES_PER_FRAME>>0)&0xFF,  // Bytes per frame (2 bytes little endian)
        (WAV_BYTES_PER_FRAME>>8)&0xFF,
        (WAV_BITS_PER_SAMPLE>>0)&0xFF,  // Bits per sample (2 bytes little endian)
        (WAV_BITS_PER_SAMPLE>>8)&0xFF,

                                        // DATA chunk
        'd', 'a', 't', 'a',             // data specifier
        0, 0, 0, 0                      // data block size
    };
    fwrite(header_format, 1, WAV_HEADER_SIZE, raw_audio_file);
}


static inline void wav_write_sample(void) {
    /* write the gameboy audio channels as 8-bit PCM integers */
    uint8_t audio_byte[4];
    audio_byte[0] = 128 + (channels[0].sample_state * 8); // scale to [128-248]
    audio_byte[1] = 128 - (channels[1].sample_state * 8); // scale to [8-128]
    audio_byte[2] = 128 + (channels[2].sample_state * 8); // scale to [128-248]
    audio_byte[3] = 128 - (channels[3].sample_state * 8); // scale to [8-128]
    fwrite(&audio_byte, 1, 4, raw_audio_file);
    wav_frames_written++;
}


void init_audio(void) {
    /* initialise the audio controller */
    ma_device_config device_config;
    ma_waveform_config master_waveform_config;

    device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = DEVICE_FORMAT;
    device_config.playback.channels = DEVICE_CHANNELS;
    device_config.sampleRate        = DEVICE_SAMPLE_RATE;
    device_config.dataCallback      = data_callback;
    device_config.pUserData         = &master_waveform;

    if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to open playback device.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Loaded audio device %s\n", device.playback.name);
    master_waveform_config = ma_waveform_config_init(device.playback.format, device.playback.channels, device.sampleRate, ma_waveform_type_square, 0.2, 240);
    ma_waveform_init(&master_waveform_config, &master_waveform);

    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start playback device.\n");
        ma_device_uninit(&device);
        exit(EXIT_FAILURE);
    }

    memset(channels, 0, sizeof(channel_attributes) * GAMEBOY_CHANNELS); // init channels attrs to 0

    if (do_export_wav) open_wav_file();
}


static inline void close_wav_file(void) {
    /* close a wav file and write file length */
    
    uint32_t data_size = htole32(wav_frames_written * WAV_BYTES_PER_FRAME);
    uint32_t file_size = htole32(WAV_HEADER_SIZE - 8 + data_size);
    fseek(raw_audio_file, 4, SEEK_SET);
    fwrite(&file_size, sizeof(uint32_t), 1, raw_audio_file);
    fseek(raw_audio_file, 40, SEEK_SET);
    fwrite(&data_size, sizeof(uint32_t), 1, raw_audio_file);
    fclose(raw_audio_file);
    fprintf(stderr, "Written %ld frames to WAV file\n", wav_frames_written);
}


void close_audio(void) {
    /* close the audio devices */
    ma_device_uninit(&device);
    ma_waveform_uninit(&master_waveform);  /* Uninitialize the waveform after the device so we don't pull it from under the device while it's being reference in the data callback. */
    if (do_export_wav) close_wav_file();
}


static inline void event_length(void) {
    /* process the ticking of the length timer */
    for (uint8_t i=0; i<4; i+=i+1) { // sequence 0,1,3 for pulse channels and wave.
        if ((read_byte(REG_NR52)&(1<<i)) && (read_byte(REG_NRx4[i])&0x40)) { // is channel i on and length enabled?
            channels[i].length_timer++;
            if (channels[i].length_timer >= 64) {
                *(ram+REG_NR52) &= ~(1<<i); // disable channel i
            }
        }
    }
    if ((read_byte(REG_NR52)&(1<<2)) && (read_byte(REG_NRx4[2])&0x40)) { // is channel 2 on and length enabled?
        channels[2].length_timer++;
        if (channels[2].length_timer >= 256) {
            *(ram+REG_NR52) &= ~(1<<2); // disable channel 2
        }
    }
}


static inline void event_ch1_freq_sweep(void) {
    /* process the ticking of channel 1's frequency sweep */
    if (read_byte(REG_NR52)&1) { // is channel on?
        uint8_t step = *(ram+REG_NR10)&7;
        bool direction = *(ram+REG_NR10)&8;
        uint8_t pace = (*(ram+REG_NR10)>>4)&7;

        uint16_t delta = channels[0].pulse_period >> step;


        if (ch1_freq_sweep_timer) {
            ch1_freq_sweep_timer--;
        } else { // timer hit 0, reset
            ch1_freq_sweep_timer = pace;
            uint16_t new_period = channels[0].pulse_period;
            if (direction) {
                new_period -= delta;
            } else {
                new_period += delta;
            }
            channels[0].pulse_period = new_period;
            if ((!direction) && new_period > 0x7FF) { // pending overflow; immediately disable
                *(ram+REG_NR52) &= ~1; // disable channel 1
                return;
            }

            if (pace) {
                *(ram+REG_NR13) = channels[0].pulse_period & 0xFF;
                *(ram+REG_NR13) &= 0xF8;
                *(ram+REG_NR13) |= channels[0].pulse_period>>8;
            }
        }
    }
}


static inline void event_envelope_sweep(void) {
    /* process the ticking of the amplitude envelope */
    for (uint8_t i=0; i<4; i+=i+1) { // sequence 0,1,3 for pulse channels and wave.
        if ((read_byte(REG_NR52)&(1<<i)) && (channels[i].amp_sweep_attrs&7)) { // is channel i on and sweep not 0 ?
            channels[i].amp_sweep_timer++;
            if (channels[i].amp_sweep_timer >= (channels[i].amp_sweep_attrs&7)) { // sweep pace
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


static inline float high_pass(float value, uint8_t cap_id) {
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
    bool dac_state[GAMEBOY_CHANNELS] = {
        read_byte(REG_NR12) & 0xF8,
        read_byte(REG_NR22) & 0xF8,
        read_byte(REG_NR30) & 128,
        read_byte(REG_NR42) & 0xF8
    };
    float curr_sample;
    for (uint8_t i=0; i<GAMEBOY_CHANNELS; i++) {
        if (dac_state[i]) {
            curr_sample = ((float)channels[i].sample_state / 30.0) - 0.25; // scale to [-0.25, 0.25]
            if (read_byte(REG_NR51) & 1 << i    ) samples[0] += curr_sample; // right
            if (read_byte(REG_NR51) & 1 << (i+4)) samples[1] += curr_sample; // left
        }
    }

    // at this point, each device channel's sample is in the range [-1,1]

    for (uint8_t i=0; i<DEVICE_CHANNELS; i++) {
        samples[i] *= (float)(((read_byte(REG_NR50) >> (4*i)) & 7) + 1) / 8.0; // scale by master volume
        samples[i] = high_pass(samples[i], i);
        write_to_buf(samples[i]);

    }
    frames_written++;
}


static void enable_channel(uint8_t channel_id) {
    /* start a pulse or noise channel */
    if (*(ram+REG_NRx4[channel_id])&0x80) { // require trigger bit is set
        *(ram+REG_NRx4[channel_id]) &= 0x7f; // reset trigger bit
        if (channel_id == 2) { // wave channel attributes
            if (!(read_byte(REG_NR30) & 128)) return; // only proceed if DAC is on
            if (channels[2].length_timer >= 256) channels[2].length_timer = *(ram + REG_NR31); // reset length timer if expired
            channels[2].amplitude = (*(ram + REG_NR32)>>5)&3;
        } else { // other channel attributes
            if (!(read_byte(REG_NRx2[channel_id]) & 0xF8)) return; // only proceed if DAC is on
            if (channels[channel_id].length_timer >= 64) channels[channel_id].length_timer = *(ram + REG_NRx1[channel_id])&63; // reset length timer if expired
            channels[channel_id].amplitude = read_byte(REG_NRx2[channel_id]) >> 4; // set sweep amplitude
            channels[channel_id].amp_sweep_timer = 0; // reset amplitude envelope timer
            channels[channel_id].amp_sweep_attrs = read_byte(REG_NRx2[channel_id]) & 15; // store amplitude sweep pace and direction
        }
        // common attributes
        *(ram+REG_NR52) |= (1<<channel_id); // enable channel
        channels[channel_id].pulse_period = *(ram+REG_NRx3[channel_id]) + (((uint16_t)(*(ram+REG_NRx4[channel_id])&7))<<8); // set pulse period
        channels[channel_id].duty_period = 0; // reset phase of pulse
    }
}


void handle_audio_register(uint16_t addr) {
    /* handle writing to an audio register. */
    switch (addr)
    {
    case REG_NR14: // channel 1
        enable_channel(0);
        ch1_freq_sweep_timer = (read_byte(REG_NR10)>>4)&7;
        break;
    case REG_NR24: // channel 2
        enable_channel(1);
        break;
    case REG_NR34: // channel 3
        enable_channel(2);
        ch3_wave_ram_index = 0;
        break;
    case REG_NR44: // channel 4
        enable_channel(3);
        ch4_LFSR = 0;
        ch4_LFSR_timer = 0;
        break;
    case REG_NR32: // channel 3 output level
        channels[2].amplitude = (*(ram + REG_NR32)>>5)&3;
        break;
    case REG_NR52:
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
            if ((channels[channel_id].duty_period&7) < duty_limit) { // sample is low
                channels[channel_id].sample_state = 0;
            } else { // sample is high
                channels[channel_id].sample_state = channels[channel_id].amplitude;
            }
            channels[channel_id].duty_period++;
        }
    }
}


static inline void tick_wave_channel(void) {
    /* process the ticking of channel 3 (wave) This function is called at a rate of 2MHz */
    if (read_byte(REG_NR52) & 4) { // is channel on?
        channels[2].pulse_period++;
        if (channels[2].pulse_period >= 0x7FF) { // overflow at 2047
            channels[2].pulse_period = *(ram+REG_NR33) + (((uint16_t)(*(ram+REG_NR34)&7))<<8); // reset to the channel period
            ch3_wave_ram_index++;
            if (ch3_wave_ram_index == 32) ch3_wave_ram_index = 0;
            ch3_buffered_sample = (read_byte(REG_WAVE1 + ch3_wave_ram_index/2) >> (4*(ch3_wave_ram_index+1)%2))&0xF;
        }
        if (channels[2].amplitude) {
            channels[2].sample_state = ch3_buffered_sample >> (channels[2].amplitude - 1);
        } else {
            channels[2].sample_state = 0;
        }
    }
}


static inline void tick_noise_channel(void) {
    /* process the ticking of channel 4 (noise). This function is called at a rate of 1MHz */
    if (read_byte(REG_NR52) & 8) { // is channel on?
        uint32_t tick_timeout = 2 * (read_byte(REG_NR43)&7);
        if (!tick_timeout) tick_timeout = 1; // treat the divider as 0.5 if the register is set to 0
        tick_timeout <<= (read_byte(REG_NR43)>>4)+1;
        ch4_LFSR_timer++;
        if (ch4_LFSR_timer >= tick_timeout) { // advance the LFSR
            ch4_LFSR_timer = 0;
            bool next_bit = (ch4_LFSR&1) == ((ch4_LFSR>>1)&1);
            ch4_LFSR &= ~(1<<15);
            ch4_LFSR |= next_bit<<15;
            if (read_byte(REG_NR43)&8) { // LFSR short mode
                ch4_LFSR &= ~(1<<7);
                ch4_LFSR |= next_bit<<7;
            }
            ch4_LFSR >>= 1;
        }
        channels[3].sample_state = channels[3].amplitude * (ch4_LFSR&1);
    }
}


void tick_audio(void) {
    /* tick every t-cycle and handle timing of all audio events */
    bool div_bit = (system_counter>>12)&1; // bit 4 of div
    if (last_div_bit && !div_bit) {

        if (div_apu % 2 == 0) event_length();           // 256 Hz
        if (div_apu % 4 == 3) event_ch1_freq_sweep();   // 128 Hz
        if (div_apu % 8 == 7) event_envelope_sweep();   // 64 Hz
        div_apu++;
    }
    if (system_counter % 2 == 0) tick_wave_channel();
    if (system_counter % 4 == 0) {
        tick_pulse_channel(0);
        tick_pulse_channel(1);
        tick_noise_channel();
    }

    if (do_export_wav && !(system_counter % (CLK_HZ / WAV_SAMPLE_RATE))) wav_write_sample();

    gb_sample_index++;
    if (gb_sample_index >= audio_sample_divider) { // send samples every 4MHz / 48000Hz samples
        queue_sample();
        gb_sample_index = 0;
    }

    last_div_bit = div_bit;
}