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
#include "audio.h"

extern uint8_t* ram;
extern uint16_t system_counter;

void tick_audio(void) {
    
}