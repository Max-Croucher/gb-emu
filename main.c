/* Main source file for the gbemu gameboy emulator.
    Author: Max Croucher 
    Email: mpccroucher@gmail.com
    May 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "cpu.h"
#include "rom.h"
#include "opcodes.h"

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
                memcpy(ram+(*reg).SP, &((*reg).PC), 2);
                printf("Got one! %d to be sent to 0x%.4x\n", isr, 0x40+(isr<<3));
                set_r16(reg, R16PC, 0x40+(isr<<3)); // go to corresponding isr addr
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    gbRom rom = load_rom(argv[1]);
    uint8_t *ram = init_ram(&rom);
    Registers reg = init_registers();

    /*Stack test with an ISR*/
    // set_ime(&reg, 1);
    // *(ram+0xFFFF) = 1; //enable VBlank interrupts
    // *(ram+0xFFac) = 0xcf;
    // set_r16(&reg, R16BC, 0x1234);
    // set_r16(&reg, R16DE, 0x89ab);
    // set_r16(&reg, R16HL, 0xeffe);
    // *(ram+(reg).PC) = 0b11000101; //push BC
    // *(ram+(reg).PC+1) = 0b11010101; //push DE
    // *(ram+(reg).PC+2) = 0b11100101; //push HL
    // *(ram+(reg).PC+3) = 0b00000001; //Load to BC
    // *(ram+(reg).PC+4) = 0b00000000; //0x0000
    // *(ram+(reg).PC+5) = 0b00000000;
    // *(ram+(reg).PC+6) = 0b00010001; //Load to DE
    // *(ram+(reg).PC+7) = 0b00000000; //0x0000
    // *(ram+(reg).PC+8) = 0b00000000;
    // *(ram+(reg).PC+9) = 0b00100001; //Load to HL
    // *(ram+(reg).PC+10) = 0b00000000; //0x0000
    // *(ram+(reg).PC+11) = 0b00000000;
    // *(ram+(reg).PC+12) = 0b00000000;
    // *(ram+(reg).PC+13) = 0b00111100; //INC A
    // *(ram+(reg).PC+14) = 0b00000000;
    // *(ram+(reg).PC+15) = 0b11000001; //Pop BC
    // *(ram+(reg).PC+16) = 0b11010001; //Pop DE
    // *(ram+(reg).PC+17) = 0b11100001; //Pop HL
    // *(ram+(reg).PC+18) = 0b11110011; //DI
    // *(ram+(reg).PC+19) = 0b00000000; //NOP
    // *(ram+(reg).PC+20) = 0b11111011; //EI
    // *(ram+(reg).PC+21) = 0b00000000; //NOP
    // *(ram+(reg).PC+22) = 0xFD; //Hard Lock
    // *(ram+0x40) = 0b00110111; //ISR entry, CSF
    // *(ram+0x41) = 0b11110000; //LDH A, imm8
    // *(ram+0x42) = 0xac;
    // *(ram+0x43) = 0b11011001;

    print_registers(&reg);

    InstructionResult instruction_result = {0,0,0,0};
    bool do_ei = 0;
    uint8_t halt_state = 0; //0 = no_halt, 1 = ime is on, 2 = no pending, 3 = pending

    int16_t machine_timeout = 0;
    for (int i=0; i<300; i++) {
        //printf("%d|%d|%d|%d\n", i, machine_timeout, reg.IME, halt_state);

        /*Interrupt for stack test*/
        // if (reg.PC == 0x0111 && !get_flag(&reg, CFLAG)) { //simulate a vblank
        //     *(ram+0xFF0F) = 1; //enable VBlank interrupts
        // }


        if (halt_state == 2 && (*(ram+0xFF0F)&*(ram+0xFFFF))) { //An interrupt is now pending to quit HALT
            halt_state = 0;
        } else if (!machine_timeout) {
            if (service_interrupts(ram, &reg)) {
                halt_state = 0; // clear halt state
                machine_timeout += 5;
            }
        }
        if ((halt_state == 0 || halt_state == 3) && !machine_timeout) {
            instruction_result = run_instruction(ram, &reg);

            if (halt_state == 3) {instruction_result.pc_offset = 0; halt_state = 0;} // instruction after HALT: Don't increment PC

            if (do_ei) set_ime(&reg, 1); // set EI late


            if (instruction_result.halt) { // HALT was called:
                if (reg.IME) {
                    halt_state = 1; // Halt until interrupt is executed
                } else if (*(ram+0xFF0F)&*(ram+0xFFFF)) {
                    halt_state = 3; // Halt until interrupt is requested
                } else {
                    halt_state = 2; // Resume, but do instruction twice
                }
                printf("Halted to %d\n", halt_state);
            }
            do_ei = instruction_result.eiset;
            machine_timeout += instruction_result.machine_cycles*4;
            reg.PC+= instruction_result.pc_offset;
            print_registers(&reg);
        }
        if (machine_timeout) {
            machine_timeout -= 1;
        }
    }

    return 0;
}