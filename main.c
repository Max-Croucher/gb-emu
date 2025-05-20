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
extern Registers reg;
extern gbRom rom;


bool service_interrupts(void) {
    /* Check if an interrupt is due, moving execution if necessary */
    if (reg.IME) { // IME must be set
        uint8_t available_regs = *(ram+0xFF0F)&*(ram+0xFFFF);
        for (int isr=0; isr<5; isr++) { //Check for each of the 5 interrupts
            if (available_regs & 1<<isr) {
                *(ram+0xFF0F) &= ~(uint8_t)(1<<isr); //disable IF
                set_ime(0); //disable isr flag
                set_isr_enable(isr, 0);
                reg.SP-=2; //execute a CALL (push PC to stack)
                write_word(reg.SP, reg.PC);
                set_r16(R16PC, 0x40+(isr<<3)); // go to corresponding isr addr
                return 1;
            }
        }
    }
    return 0;
}


bool increment_timers(uint16_t machine_ticks) {
    /* Handle the incrementing and overflowing of timers */
    if (!(machine_ticks&0x00FF)) (*(ram+0xFF04))++; //DIV
    uint8_t TAC = *(ram+0xFF07);
    bool trigger = 0;
    bool do_interrupt = 0;
    if (TAC&4) { // Is timer enabled
        switch (TAC&3) // Select timer speed
        {
        case 0: //4096 Hz
            if (!(machine_ticks&0x03FF)) {(*(ram+0xFF05))++; trigger=1;}
            break;
        case 3: //16384 Hz
            if (!(machine_ticks&0x00FF)) {(*(ram+0xFF05))++; trigger=1;}
            break;
        case 2: //65536 Hz
            if (!(machine_ticks&0x003F)) {(*(ram+0xFF05))++; trigger=1;}
            break;
        case 1: //262144 Hz
            if (!(machine_ticks&0x000F)) {(*(ram+0xFF05))++; trigger=1;}
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
    InstructionResult instruction_result = {0,0,0,0};
    
    FILE *logfile;
	logfile = fopen("cpu_states.log", "w");

    bool do_ei = 0;
    uint8_t halt_state = 0; //0 = no_halt, 1 = ime is on, 2 = no pending, 3 = pending
    int16_t machine_timeout = 0;
    uint16_t machine_ticks = 0;
    uint64_t frames = 0;

    //print_registers();
    uint16_t count = 0;
    while (1) {
        machine_ticks++;
        if (machine_ticks == 0) count += 1;
        if (count == (8192)) break; // 64 cycles is one second
        //if ((count == 1024)) break;
        //12 frames is enough for Tetris's tilemap to fully load
        //printf("%d|%d|%d|%d\n", i, machine_timeout, reg.IME, halt_state);

        if (increment_timers(machine_ticks)) {
            if (TIMA_oddity) {
                TIMA_oddity = 0;
            } else {
                *(ram+0xFF05) = *(ram+0xFF06); // reset to TMA
                *(ram+0xFF0F) |= 1<<2; // request a timer interrupt
            }
        }

            if (!machine_timeout) {
                fprintf(logfile, "A:%.2x F:%.2x B:%.2x C:%.2x D:%.2x E:%.2x H:%.2x L:%.2x SP:%.4x PC:%.4x PCMEM:%.2x,%.2x,%.2x,%.2x IME:%d HALTMODE:%d INTFLAGS:%.2x OPCODES: %s\n",
                get_r8(R8A),get_r8(R8F),get_r8(R8B),get_r8(R8C),
                get_r8(R8D),get_r8(R8E),get_r8(R8H),get_r8(R8L),
                get_r16(R16SP),get_r16(R16PC),
                *(ram+get_r16(R16PC)),*(ram+get_r16(R16PC)+1),*(ram+get_r16(R16PC)+2),*(ram+get_r16(R16PC)+3),
                reg.IME, halt_state, *(ram+0xFF0F),
                (*(ram+get_r16(R16PC))==0xCB) ? mn_opcodes[*(ram+get_r16(R16PC))] : mn_cb_opcodes[*(ram+get_r16(R16PC)+1)]
            );

            if (halt_state == 2 && (*(ram+0xFF0F))) { //An interrupt is now pending to quit HALT
                halt_state = 0;
            } else {
                if (service_interrupts()) {
                    halt_state = 0; // clear halt state
                    machine_timeout += 20;
                    //printf("ISR\n");
                }
            }
            if (halt_state == 0 || halt_state == 3) {
                instruction_result = run_instruction();

                //if (halt_state == 2) {instruction_result.new_pc = get_r16(R16PC); halt_state = 0;} // instruction after HALT: Don't increment PC
                if (halt_state == 3) halt_state = 0;
                if (do_ei) set_ime(1); // set EI late

                if (instruction_result.halt) { // HALT was called:
                    if (reg.IME) {
                        halt_state = 1; // Halt until interrupt is executed
                    } else if (*(ram+0xFF0F)&0x1F) {
                        halt_state = 3; // Resume, but do instruction twice
                    } else {
                        halt_state = 2; // Halt until interrupt is requested
                    }
                }
                do_ei = instruction_result.eiset;
                machine_timeout += instruction_result.machine_cycles*4;
                reg.PC = instruction_result.new_pc;
                //print_registers();
            }
        } else {
            machine_timeout -= 1;
        }
        frames += tick_graphics();
        //usleep(10);
    }
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