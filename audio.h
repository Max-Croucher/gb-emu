/* Header file for audio.c, emulating gameboy audio
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  June 2025
*/

#ifndef AUDIO_H
#define AUDIO_H

#define MA_NO_DECODING
#define MA_NO_ENCODING

typedef struct channel_attributes {
    uint8_t sample_state;
    uint8_t length_timer;
    uint8_t amp_sweep_timer;
    uint8_t amp_sweep_attrs;
    uint8_t amplitude;
    uint8_t duty_period;
    uint16_t pulse_period;

}channel_attributes;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void init_audio(void);
void close_audio(void);
static inline void event_length(void);
static inline void event_ch1_freq_sweep(void);
static inline void event_envelope_sweep(void);
static inline float high_pass(float value, uint8_t cap_id);
static inline void write_to_buf(float val);
static inline void queue_sample(void);
static void enable_channel(uint8_t channel_id);
void handle_audio_register(uint16_t addr);
static inline void tick_pulse_channel(bool channel_id);
static inline void tick_wave_channel(void);
static inline void tick_noise_channel(void);
void tick_audio(void);

#endif //AUDIO