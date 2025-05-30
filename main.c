/* Main source file for the gbemu gameboy emulator.
    Author: Max Croucher 
    Email: mpccroucher@gmail.com
    May 2025
*/

/*
Notes:
    - implement STOP behavior
    - implement io
    - implement graphics
    - implement bank switching & other cartridge features
    - implement strange div write behaviour


*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <GL/freeglut.h>
#include "cpu.h"
#include "rom.h"
#include "opcodes.h"
#include "graphics.h"
#include "mnemonics.h"

#include <unistd.h>


bool TIMA_oddity = 0; //extern
uint8_t* ram; //extern
uint16_t system_counter = 0xABD6; //extern
extern Registers reg;
extern gbRom rom;
extern bool LOOP;
extern bool OAM_DMA;
extern uint16_t OAM_DMA_timeout;
extern bool do_ei_set;
extern uint8_t do_haltmode;
extern void (*scheduled_instructions[10])(void);
extern uint8_t num_scheduled_instructions;
extern uint8_t current_instruction_count;


bool service_interrupts(void) {
    /* Check if an interrupt is due, moving execution if necessary */
    if (reg.IME) { // IME must be set
        uint8_t available_regs = *(ram+0xFF0F)&*(ram+0xFFFF);
        for (int isr=0; isr<5; isr++) { //Check for each of the 5 interrupts
            if (available_regs & 1<<isr) {
                *(ram+0xFF0F) &= ~(uint8_t)(1<<isr); //disable IF
                set_ime(0);
                load_interrupt_instructions(isr);
                return 1;
            }
        }
    }
    return 0;
}


bool increment_timers() {
    /* Handle the incrementing and overflowing of timers */
    uint8_t TAC = *(ram+0xFF07);
    bool trigger = 0;
    bool do_interrupt = 0;
    if (TAC&4) { // Is timer enabled
        switch (TAC&3) // Select timer speed
        {
        case 0: //4096 Hz
            if (!(system_counter&0x03FF)) {(*(ram+0xFF05))++; trigger=1;}
            break;
        case 3: //16384 Hz
            if (!(system_counter&0x00FF)) {(*(ram+0xFF05))++; trigger=1;}
            break;
        case 2: //65536 Hz
            if (!(system_counter&0x003F)) {(*(ram+0xFF05))++; trigger=1;}
            break;
        case 1: //262144 Hz
            if (!(system_counter&0x000F)) {(*(ram+0xFF05))++; trigger=1;}
            break;
        }
        //printf("TIMA 0x%.4x\n", *(ram+0xFF05));
        if (trigger && !*(ram+0xFF05)) { //TIMA overflows
            do_interrupt = 1;
        }
    }
    return do_interrupt;
}


int main(int argc, char *argv[]) {
    load_rom(argv[1]);
    init_ram();
    init_registers();
    init_graphics(&argc, argv);
    
    FILE *logfile;
	logfile = fopen("cpu_states.log", "w");
    int8_t do_ei = 0;
    bool halt_state = 0;
    bool stop_mode = 0;
    uint64_t frames = 0;
    bool debug_skip_state = 0;

    uint16_t count = 0;
    while (LOOP) {
        system_counter++;
        //fprintf(stderr, "%d | (%d, %d) | %d | %d\n", system_counter, current_instruction_count, num_scheduled_instructions, halt_state, stop_mode);
        if (increment_timers()) {
            if (TIMA_oddity) {
                TIMA_oddity = 0;
            } else {
                *(ram+0xFF05) = *(ram+0xFF06); // reset to TMA
                *(ram+0xFF0F) |= 1<<2; // request a timer interrupt
            }
        }
        if (stop_mode) {
            if ((*(ram+0xFF00)&0xF) != 0xF) stop_mode = 0; 
        } else {

            if (OAM_DMA) {
                if ((OAM_DMA_timeout&3) == 0) {
                    uint8_t lower_addr = 160 - (OAM_DMA_timeout>>2);
                    *(ram + 0xFE00 + lower_addr) = *(ram + ((uint16_t)(*(ram+0xFF46))<<8) + lower_addr);
                }
                OAM_DMA_timeout -= 1;
                if (!OAM_DMA_timeout) OAM_DMA = 0;
            }

            if (!(system_counter&3)) {
                if (current_instruction_count < num_scheduled_instructions) {
                    scheduled_instructions[current_instruction_count]();
                    current_instruction_count++;


                    if (do_haltmode) { // HALT was called:
                        if (do_haltmode == 2) {
                            stop_mode = 1;
                        } else {
                            halt_state = 1;
                        }
                        do_haltmode = 0;
                    }
                    if (do_ei_set && (do_ei == 0)) {
                        do_ei = 2;
                        do_ei_set = 0;
                    }


                } else {
                    if (do_ei > 0) {
                        do_ei--;
                        if (!do_ei)set_ime(1); // set EI late
                    }

                    // fprintf(logfile, "A:%.2x F:%.2x B:%.2x C:%.2x D:%.2x E:%.2x H:%.2x L:%.2x SP:%.4x PC:%.4x PCMEM:%.2x,%.2x,%.2x,%.2x IME:%d HALTMODE:%d STOP:%d IE:%.2x IF:%.2x OPCODES: %s\n",
                    //     get_r8(R8A),get_r8(R8F),get_r8(R8B),get_r8(R8C),
                    //     get_r8(R8D),get_r8(R8E),get_r8(R8H),get_r8(R8L),
                    //     get_r16(R16SP),get_r16(R16PC),
                    //     *(ram+get_r16(R16PC)),*(ram+get_r16(R16PC)+1),*(ram+get_r16(R16PC)+2),*(ram+get_r16(R16PC)+3),
                    //     reg.IME, halt_state, stop_mode, *(ram+0xFFFF), *(ram+0xFF0F),
                    //     ((*(ram+get_r16(R16PC))==0xCB) ? mn_cb_opcodes[*(ram+get_r16(R16PC)+1)] : mn_opcodes[*(ram+get_r16(R16PC))])
                    // );

                    if (halt_state && (*(ram+0xFF0F)&*(ram+0xFFFF))) { //An interrupt is now pending to quit HALT
                        halt_state = 0;
                    }
                    if (!halt_state) service_interrupts();

                    if ((!halt_state) && (current_instruction_count >= num_scheduled_instructions)) {
                        queue_instruction();
                    }
                }
            }
            frames += tick_graphics();
            //usleep(10);
        }
    }
    if (!LOOP) fprintf(stderr, "Exiting (keypress).\n");
    fprintf(stderr,"Processed %ld frames.\n", frames);

    //write ram contents to a file
    FILE *f;
	f = fopen("ram_contents.hex", "wb");
    fwrite(ram, 1, 0x10000, f);
    fprintf(stderr, "written RAM to 'ram_contents.hex'.\n");
    fclose(f);
    fclose(logfile);

    //free mallocs
    free(rom.rom);
    free(rom.ram);
    free(ram);

    return 0;
}