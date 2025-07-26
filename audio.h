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
    uint8_t amp_sweep_timer;
    uint8_t amp_sweep_attrs;
    uint8_t amplitude;
    uint8_t duty_period;
    uint16_t length_timer;
    uint16_t pulse_period;

}channel_attributes;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
void init_audio(void);
void close_audio(void);
void handle_audio_register(uint16_t addr);
void tick_audio(void);

#endif //AUDIO