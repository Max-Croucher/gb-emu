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

extern uint8_t* ram;
extern uint16_t system_counter;
uint8_t div_apu = 1;
bool last_sysclk_bit = 0;

ma_result result;
ma_engine engine;

bool init_audio(void) {
    result = ma_engine_init(NULL, &engine);
    return (result == MA_SUCCESS);
}


void event_length(void) {
    
}


void event_ch1_freq_sweep(void) {
    
}


void event_envelope_sweep(void) {
    
}


void tick_audio(void) {
    bool sysclk_bit = (system_counter>>4)&1;
    if (last_sysclk_bit && !sysclk_bit) {
        div_apu++;

        if (div_apu % 2 == 0) event_length();
        if (div_apu % 4 == 0) event_ch1_freq_sweep();
        if (div_apu == 8) {
            event_envelope_sweep();
            div_apu = 0;
        }

    }
    last_sysclk_bit = sysclk_bit;
}