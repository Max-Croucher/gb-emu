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

uint8_t block00(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b00 */
    uint8_t offset_pc = 0;
    uint16_t imm16;
    uint8_t imm8;
    switch (opcode & 0xF)
    {
    case 0: //NOP
        offset_pc = 1;
        break;
    case 1: //LD r16, imm16 | load 16 bit value imm16 into r16
        offset_pc = 3;
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        set_r16(reg, (opcode>>4)&3, imm16);
        break;
    case 2: //LD [r16], A | load contents of r8A to the byte pointed to by [r16]
        offset_pc = 1;
        *(ram+get_r16(reg, (opcode>>4)&3)) = get_r8(reg, R8A);
        break;
    case 10: //LD A, [r16] | load byte pointed to by address [r16] into LDA
        offset_pc = 1;
        set_r8(reg, R8A, *(ram+get_r16(reg, (opcode>>4)&3)));
        break;
    case 8: //LD [imm16], SP | load bytes in SP to the bytes pointed to by address [r16] and [r16+1]
        offset_pc = 3;
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        printf("Got %.4x\n", imm16);
        memcpy(ram+imm16, &(*reg).SP, 2);
        break;
    case 3: //INC r16 | increment r16
        offset_pc = 1;
        set_r16(reg, (opcode>>4)&3, get_r16(reg,(opcode>>4)&3)+1);
        break;
    case 11: //DEC r16 | decrement r16
        offset_pc = 1;
        set_r16(reg, (opcode>>4)&3, get_r16(reg,(opcode>>4)&3)-1);
        break;
    case 9: //ADD HL, r16 | add value in r16 to HL
        offset_pc = 1;
        uint16_t old_hl = get_r16(reg, R16HL);
        set_r16(reg, R16HL,old_hl+get_r16(reg, (opcode>>4)&3));
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, get_r16(reg, R16HL)<old_hl);
        set_flag(reg, HFLAG, (old_hl&0x0FFF)+(get_r16(reg, (opcode>>4)&3)&0x0FFF)>=0x1000);
        break;
    case 4:
    case 12: // INC r8 | increment r8
        offset_pc = 1;
        set_r8(reg, (opcode>>3)&7, get_r8(reg, (opcode>>3)&7)+1);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode>>3)&7)==0);
        set_flag(reg, HFLAG, get_r8(reg, (opcode>>3)&7)==16);
        break;
    case 5:
    case 13: // DEC r8 | decrement r8
        offset_pc = 1;
        set_r8(reg, (opcode>>3)&7, get_r8(reg, (opcode>>3)&7)-1);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode>>3)&7)==0);
        set_flag(reg, HFLAG, get_r8(reg, (opcode>>3)&7)==15);
        break;
        
    default:; //Unknown opcode
        char buf[64];
        sprintf(buf, "Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
        break;
    }
    return offset_pc;
}

uint8_t block01(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b01 */
    uint8_t offset_pc = 0;
    uint16_t imm16;
    uint8_t imm8;
        char buf[64];
        sprintf(buf, "Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
    return offset_pc;
}

uint8_t block10(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b10 */
    uint8_t offset_pc = 0;
    uint16_t imm16;
    uint8_t imm8;
        char buf[64];
        sprintf(buf, "Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
    return offset_pc;
}

uint8_t block11(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b11 */
    uint8_t offset_pc = 0;
    uint16_t imm16;
    uint8_t imm8;
        char buf[64];
        sprintf(buf, "Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
    return offset_pc;
}

void run_instruction(gbRom* rom, uint8_t* ram, Registers* reg) {
    /* read the opcode at the PC and execute an instruction */
    uint8_t opcode = *(ram+(*reg).PC);
    printf("PC=0x%.4x OPCODE=0x%.2x BITS=",(*reg).PC, opcode);
    for (int8_t i=7; i>=0; i--) {
        printf("%d", (opcode>>i) & 1);
    }
    printf("\n");
    uint8_t inc=0;
    switch (opcode>>6)
    {
    case 0:
        inc = block00(rom, ram, reg, opcode);
        break;
    case 1:
        inc = block01(rom, ram, reg, opcode);
        break;
    case 2:
        inc = block10(rom, ram, reg, opcode);
        break;
    case 3:
        inc = block11(rom, ram, reg, opcode);
        break;
    }
    (*reg).PC+= inc;
}

int main(int argc, char *argv[]) {
    gbRom rom = load_rom(argv[1]);
    uint8_t *ram = init_ram(&rom);
    Registers reg = init_registers();

    set_r8(&reg, R8A, 1);
    *(ram+(reg).PC) = 0b00111101;
    *(ram+(reg).PC+1) = 0b00111101;
    *(ram+(reg).PC+2) = 0b00111101;
    *(ram+(reg).PC+3) = 0b00111101;


    for (int i=0; i<10; i++) {
        run_instruction(&rom, ram, &reg);
        print_registers(&reg);
    }

    return 0;
}