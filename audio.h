/* Header file for audio.c, emulating gameboy audio
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  June 2025
*/

#ifndef AUDIO_H
#define AUDIO_H

#define MA_NO_DECODING
#define MA_NO_ENCODING

bool init_audio(void);
void event_length(void);
void event_ch1_freq_sweep(void);
void event_envelope_sweep(void);
void tick_audio(void);

#endif //AUDIO