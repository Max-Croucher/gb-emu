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


uint8_t* ram; //extern
FILE *logfile;
int8_t do_ei = 0;
bool halt_state = 0;
bool stop_mode = 0;

bool halt_on_breakpoint = 0; //extern
bool print_breakpoints = 0; //extern
bool hyperspeed = 0;
bool no_display = 0;
bool verbose_logging = 0;

extern uint16_t system_counter;
extern uint8_t TIMA_overflow_delay;
extern Registers reg;
extern gbRom rom;
extern bool LOOP;
extern bool OAM_DMA;
extern uint8_t OAM_DMA_starter;
extern bool OAM_DMA_timeout;
extern bool TIMA_overflow_flag;
extern bool do_ei_set;
extern uint8_t do_haltmode;
extern void (*scheduled_instructions[10])(void);
extern uint8_t num_scheduled_instructions;
extern uint8_t current_instruction_count;


void decode_launch_args(int argc, char *argv[]) {
    /* read argv to set launch arguments */
    for (int i=2; i<argc; i++) {
        if (!strcmp(argv[i], "--halt-on-breakpoints")) halt_on_breakpoint = 1;
        if (!strcmp(argv[i], "--print-breakpoints")) print_breakpoints = 1;
        if (!strcmp(argv[i], "--max-speed")) hyperspeed = 1;
        if (!strcmp(argv[i], "--windowless")) no_display = 1;
        if (!strcmp(argv[i], "--debug")) verbose_logging = 1;
    }
}


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


int main(int argc, char *argv[]) {
    load_rom(argv[1]);
    decode_launch_args(argc, argv);
    init_ram();
    init_registers();
    if (!no_display) init_graphics(&argc, argv, rom.title);
    if (verbose_logging) {
        logfile = fopen("cpu_states.log", "w");
    }

    while (LOOP) {
        //fprintf(stderr, "%d | (%d, %d) | %d | %d\n", system_counter, current_instruction_count, num_scheduled_instructions, halt_state, stop_mode);
        increment_timers();
        if (stop_mode) {
            if ((*(ram+0xFF00)&0xF) != 0xF) stop_mode = 0; 
        } else {

            if (!(system_counter&3)) {
                if (OAM_DMA_starter) {
                    OAM_DMA_starter--;
                    if (!OAM_DMA_starter) {
                        fprintf(stderr, "DMA: starting at sysclk=0x%.4x\n", system_counter);
                        printf("DMA: starting at sysclk=0x%.4x\n", system_counter);
                        OAM_DMA = 1;
                        OAM_DMA_timeout = 0;
                    }
                }
                if (OAM_DMA) read_dma();

                if (TIMA_overflow_delay) { // do TIMA overflow late
                    TIMA_overflow_delay--;
                    if ((!*(ram+0xFF05)) && (!TIMA_overflow_delay)) {
                        *(ram+0xFF05) = *(ram+0xFF06); // reset to TMA
                        *(ram+0xFF0F) |= 1<<2; // request a timer interrupt
                        TIMA_overflow_flag = 1;
                    }
                }


                if (current_instruction_count == num_scheduled_instructions) {
                    if (do_ei > 0) {
                        do_ei--;
                        if (!do_ei)set_ime(1); // set EI late
                    }


                    if (verbose_logging) fprintf(logfile, "A:%.2x F:%.2x B:%.2x C:%.2x D:%.2x E:%.2x H:%.2x L:%.2x SP:%.4x PC:%.4x PCMEM:%.2x,%.2x,%.2x,%.2x IME:%d HALTMODE:%d STOP:%d IE:%.2x IF:%.2x OPCODES: %s\n",
                        get_r8(R8A),get_r8(R8F),get_r8(R8B),get_r8(R8C),
                        get_r8(R8D),get_r8(R8E),get_r8(R8H),get_r8(R8L),
                        get_r16(R16SP),get_r16(R16PC),
                        *(ram+get_r16(R16PC)),*(ram+get_r16(R16PC)+1),*(ram+get_r16(R16PC)+2),*(ram+get_r16(R16PC)+3),
                        reg.IME, halt_state, stop_mode, *(ram+0xFFFF), *(ram+0xFF0F),
                        ((*(ram+get_r16(R16PC))==0xCB) ? mn_cb_opcodes[*(ram+get_r16(R16PC)+1)] : mn_opcodes[*(ram+get_r16(R16PC))])
                    );

                    if (halt_state && (*(ram+0xFF0F)&*(ram+0xFFFF))) { //An interrupt is now pending to quit HALT
                        halt_state = 0;
                    }
                    if (!halt_state) service_interrupts();

                    if ((!halt_state) && (current_instruction_count == num_scheduled_instructions)) {
                        queue_instruction();
                    }
                }


                // printf("\ncount %d/%d | sysclk=0x%.4x", current_instruction_count, num_scheduled_instructions, system_counter);
                // if (current_instruction_count == 0) {
                //     printf(" | OPCODE=0x%.2x INSTR=%s\n", *(ram+get_r16(R16PC)), ((*(ram+get_r16(R16PC))==0xCB) ? mn_cb_opcodes[*(ram+get_r16(R16PC)+1)] : mn_opcodes[*(ram+get_r16(R16PC))]));
                // } else {
                //     printf("\n");
                // }

                if (!halt_state) {
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
                }
                TIMA_overflow_flag = 0;
            }
            if (!no_display) tick_graphics();
            //usleep(10);
        }
    }

    //write ram contents to a file
    if (verbose_logging) {
        FILE *f;
        f = fopen("ram_contents.hex", "wb");
        fwrite(ram, 1, 0x10000, f);
        fprintf(stderr, "written RAM to 'ram_contents.hex'.\n");
        fclose(f);
        fclose(logfile);
    }

    //free mallocs
    free(rom.rom);
    free(rom.ram);
    free(ram);

    return 0;
}