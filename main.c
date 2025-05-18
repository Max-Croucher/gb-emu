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

#include <unistd.h>


extern bool TIMA_oddity;
bool tset = 0;

bool service_interrupts(uint8_t* ram, Registers* reg) {
    /* Check if an interrupt is due, moving execution if necessary */
    if ((*reg).IME) { // IME must be set
        uint8_t available_regs = *(ram+0xFF0F)&*(ram+0xFFFF);
        for (int isr=0; isr<5; isr++) { //Check for each of the 5 interrupts
            if (available_regs & 1<<isr) {
                *(ram+0xFF0F) &= ~(uint8_t)(1<<isr); //disable IF
                set_ime(reg, 0); //disable isr flag
                set_isr_enable(ram, isr, 0);
                (*reg).SP-=2; //execute a CALL (push PC to stack)
                write_word(ram, (*reg).SP, (*reg).PC);
                set_r16(reg, R16PC, 0x40+(isr<<3)); // go to corresponding isr addr
                return 1;
            }
        }
    }
    return 0;
}


bool increment_timers(uint8_t* ram, uint16_t machine_ticks) {
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
            tset = 1;
            do_interrupt = 1;
        }
    }
    return do_interrupt;
}


int main(int argc, char *argv[]) {
    init_graphics(&argc, argv);
    gbRom rom = load_rom(argv[1]);
    uint8_t *ram = init_ram(&rom);
    Registers reg = init_registers();
    InstructionResult instruction_result = {0,0,0,0};
    
    bool do_ei = 0;
    uint8_t halt_state = 0; //0 = no_halt, 1 = ime is on, 2 = no pending, 3 = pending
    int16_t machine_timeout = 0;
    uint16_t machine_ticks = 0;

    //print_registers(&reg);
    uint16_t count = 0;
    while (1) {
        machine_ticks++;
        if (!machine_ticks) count += 1;
        if (count == 256) break;
        //printf("%d|%d|%d|%d\n", i, machine_timeout, reg.IME, halt_state);

        if (increment_timers(ram, machine_ticks)) {
            if (TIMA_oddity) {
                TIMA_oddity = 0;
            } else {
                *(ram+0xFF05) = *(ram+0xFF06); // reset to TMA
                *(ram+0xFF0F) |= 1<<2; // request a timer interrupt
            }
        }

            if (!machine_timeout) {
                printf("A:%.2x F:%.2x B:%.2x C:%.2x D:%.2x E:%.2x H:%.2x L:%.2x SP:%.4x PC:%.4x PCMEM:%.2x,%.2x,%.2x,%.2x IME:%d TIME:%d HALTMODE:%d INTFLAGS:%.2x",
                get_r8(&reg, ram, R8A),get_r8(&reg, ram, R8F),get_r8(&reg, ram, R8B),get_r8(&reg, ram, R8C),
                get_r8(&reg, ram, R8D),get_r8(&reg, ram, R8E),get_r8(&reg, ram, R8H),get_r8(&reg, ram, R8L),
                get_r16(&reg, R16SP),get_r16(&reg, R16PC),
                *(ram+get_r16(&reg, R16PC)),*(ram+get_r16(&reg, R16PC)+1),*(ram+get_r16(&reg, R16PC)+2),*(ram+get_r16(&reg, R16PC)+3),
                reg.IME, tset, halt_state, *(ram+0xFF0F)
            );
            tset = 0;

            if (halt_state == 2 && (*(ram+0xFF0F))) { //An interrupt is now pending to quit HALT
                halt_state = 0;
            } else {
                if (service_interrupts(ram, &reg)) {
                    halt_state = 0; // clear halt state
                    machine_timeout += 20;
                    //printf("ISR\n");
                }
            }
            if (halt_state == 0 || halt_state == 3) {
                instruction_result = run_instruction(ram, &reg);

                //if (halt_state == 2) {instruction_result.new_pc = get_r16(&reg, R16PC); halt_state = 0;} // instruction after HALT: Don't increment PC
                if (halt_state == 3) halt_state = 0;
                if (do_ei) set_ime(&reg, 1); // set EI late

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
                //print_registers(&reg);
            }
        } else {
            machine_timeout -= 1;
        }
        //tick_graphics(ram);
        //usleep(10);
    }

    return 0;
}