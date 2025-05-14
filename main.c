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
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;
    //explicit 8-bit opcodes
    switch (opcode)
    {
    case 0b00001000: //LD [imm16], SP | load bytes in SP to the bytes pointed to by address [r16] and [r16+1]
        offset_pc = 3;
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        printf("Got %.4x\n", imm16);
        memcpy(ram+imm16, &(*reg).SP, 2);
        break;
    case 0b00000111: //RLCA | bit-shift rotate register A left, store wraparound into C
        offset_pc = 1;
        set_flag(reg, CFLAG, get_r8(reg, R8A)>>7);
        working8bit = (get_r8(reg, R8A) << 1) + get_flag(reg, CFLAG);
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00001111: //RRCA | bit-shift rotate register A right, store wraparound into C
        offset_pc = 1;
        set_flag(reg, CFLAG, get_r8(reg, R8A)&1);
        working8bit = (get_r8(reg, R8A) >> 1) + (get_flag(reg, CFLAG)<<7);
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00010111: //RLA | bit-shift rotate register A left, through C
        offset_pc = 1;
        workingflag = get_flag(reg, CFLAG);
        set_flag(reg, CFLAG, get_r8(reg, R8A)>>7);
        working8bit = (get_r8(reg, R8A) << 1) + workingflag;
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00011111: //RRA | bit-shift rotate register A right, through C
        offset_pc = 1;
        workingflag = get_flag(reg, CFLAG);
        set_flag(reg, CFLAG, get_r8(reg, R8A)&1);
        working8bit = (get_r8(reg, R8A) >> 1) + (workingflag<<7);
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00100111: //DAA | Decimal adjust accumulator
        offset_pc = 1;
        uint8_t daa_adj = 0;
        if (get_flag(reg, NFLAG)) {
            if (get_flag(reg, HFLAG)) daa_adj+= 0x06;
            if (get_flag(reg, CFLAG)) daa_adj+= 0x60;
            set_r8(reg, R8A, get_r8(reg, R8A) - daa_adj);
        } else {
            if (get_flag(reg, HFLAG) || (get_r8(reg, R8A)&0x0F)>0x09) daa_adj+= 0x06;
            if (get_flag(reg, CFLAG) || get_r8(reg, R8A)>0x99) {daa_adj+= 0x60; set_flag(reg, CFLAG, 1);}
            set_r8(reg, R8A, get_r8(reg, R8A) + daa_adj);
        }
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, get_flag(reg, CFLAG) || get_r8(reg, R8A)>0x99);
        break;
    case 0b00101111: //CPL | bitwise not
        offset_pc = 1;
        set_r8(reg, R8A, ~get_r8(reg, R8A));
        set_flag(reg, NFLAG, 1);
        set_flag(reg, HFLAG, 1);
        break;
    case 0b00110111: //SCF | set carry flag
        offset_pc = 1;
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 1);
        break;
    case 0b00111111: //CCF | complement carry flag
        offset_pc = 1;
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, !get_flag(reg, CFLAG));
        break;
    case 0b00011000: //jr imm8 | relative jump to SIGNED byte in imm8
        offset_pc = 0;
        int8_t relative_addr;
        memcpy(&relative_addr, ram+(*reg).PC+1, 1);
        set_r16(reg, R16PC, get_r16(reg, R16PC)+relative_addr);
        break;
    case 0b00100000:
    case 0b00101000:
    case 0b00110000:
    case 0b00111000: //jr cond; imm8 | relative conditional jump to SIGNED byte in imm8
        if (is_cc(reg, (opcode>>3)&3)) {
            offset_pc = block00(rom, ram, reg, 0b00011000); //call jr imm8
        } else {
            offset_pc = 2;
        }
        break;
    case 0b00010000: //STOP
        offset_pc = 2;
        print_error("STOP is not implemented!");
        break;
    default:
        has_finished = 0;
    }
    if (has_finished) return offset_pc;
    //implicit opcodes, controlled by mask 0b00001111
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
    case 12: //INC r8 | increment r8
        offset_pc = 1;
        set_r8(reg, (opcode>>3)&7, get_r8(reg, (opcode>>3)&7)+1);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode>>3)&7)==0);
        set_flag(reg, HFLAG, get_r8(reg, (opcode>>3)&7)==16);
        break;
    case 5:
    case 13: //DEC r8 | decrement r8
        offset_pc = 1;
        set_r8(reg, (opcode>>3)&7, get_r8(reg, (opcode>>3)&7)-1);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode>>3)&7)==0);
        set_flag(reg, HFLAG, get_r8(reg, (opcode>>3)&7)==15);
        break;
    case 6:
    case 14: //LD r8, imm8 | load value imm8 into r8
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, (opcode>>3)&7, imm8);
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
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    if (opcode == 0b01110110) { //HALT
        print_error("STOP is not implemented!");
    } else { //LD r8, r8 | load r8 into r8
        offset_pc = 1;
        set_r8(reg, (opcode>>3)&7, get_r8(reg, opcode&7));
    }

    return offset_pc;
}

uint8_t block10(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b10 */
    uint8_t offset_pc = 0;
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    switch ((opcode>>3)&7)
    {
    case 0: //ADD A, r8 | add contents of r8 to A
        offset_pc = 1;
        set_r8(reg, R8A, get_r8(reg, R8A) + get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, get_r8(reg, R8A)<get_r8(reg, opcode&7));
        set_flag(reg, HFLAG, (get_r8(reg, R8A)&15)<(get_r8(reg, opcode&7)&15));
        break;
    case 1: //ADC A, r8 | add carry and contents of r8 to A
        offset_pc = 1;
        working16bit = get_r8(reg, R8A) + get_r8(reg, opcode&7) + get_flag(reg, CFLAG);
        working8bit = get_r8(reg, R8A)&15 + (get_r8(reg, opcode&7)&15) + get_flag(reg, CFLAG);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 2: //SUB A, r8 | subtract contents of r8 from A
        offset_pc = 1;
        working16bit = get_r8(reg, R8A) - get_r8(reg, opcode&7);
        working8bit = (get_r8(reg, R8A)&15) - (get_r8(reg, opcode&7)&15);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 3: //SBC A, r8 | subtract carry and contents of r8 to A
        offset_pc = 1;
        working16bit = get_r8(reg, R8A) - get_r8(reg, opcode&7) - get_flag(reg, CFLAG);
        working8bit = (get_r8(reg, R8A)&15) - (get_r8(reg, opcode&7)&15) - get_flag(reg, CFLAG);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 4: //AND A, r8 | logical and between r8 and A
        offset_pc = 1;
        set_r8(reg, R8A, get_r8(reg, R8A) & get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 1);
        set_flag(reg, CFLAG, 0);
        break;
    case 5: //XOR A, r8 | logical xor between r8 and A
        offset_pc = 1;
        set_r8(reg, R8A, get_r8(reg, R8A) ^ get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 6: //OR A, r8 | logical or between r8 and A
        offset_pc = 1;
        set_r8(reg, R8A, get_r8(reg, R8A) | get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 7: //CP A, r8 | subtract contents of r8 from A but discard result
        offset_pc = 1;
        working16bit = get_r8(reg, R8A) - get_r8(reg, opcode&7);
        working8bit = (get_r8(reg, R8A)&15) - (get_r8(reg, opcode&7)&15);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    default:;
        bool workingflag;
        char buf[64];
        sprintf(buf, "Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
    }
    return offset_pc;
}

uint8_t block11(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b11 */
    uint8_t offset_pc = 0;
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    switch (opcode)
    {
    case 0b11000110: //ADD A, imm8 | add contents of imm8 to A
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) + imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, get_r8(reg, R8A)<imm8);
        set_flag(reg, HFLAG, (get_r8(reg, R8A)&15)<(imm8&15));
        break;
    case 0b11001110: //ADC A, imm8 | add carry and contents of imm8 to A
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        working16bit = get_r8(reg, R8A) + imm8 + get_flag(reg, CFLAG);
        working8bit = get_r8(reg, R8A)&15 + (imm8&15) + get_flag(reg, CFLAG);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 0b11010110: //SUB A, imm8 | subtract contents of imm8 from A
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        working16bit = get_r8(reg, R8A) - imm8;
        working8bit = (get_r8(reg, R8A)&15) - (imm8&15);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 0b11011110: //SBC A, imm8 | subtract carry and contents of imm8 to A
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        working16bit = get_r8(reg, R8A) - imm8 - get_flag(reg, CFLAG);
        working8bit = (get_r8(reg, R8A)&15) - (imm8&15) - get_flag(reg, CFLAG);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 0b11100110: //AND A, imm8 | logical and between imm8 and A
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) & imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 1);
        set_flag(reg, CFLAG, 0);
        break;
    case 0b11101110: //XOR A, imm8 | logical xor between imm8 and A
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) ^ imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 0b11110110: //OR A, imm8 | logical or between imm8 and A
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) | imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 0b11111110: //CP A, imm8 | subtract contents of imm8 from A but discard result
        offset_pc = 2;
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        working16bit = get_r8(reg, R8A) - imm8;
        working8bit = (get_r8(reg, R8A)&15) - (imm8&15);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 0b11000000:
    case 0b11001000:
    case 0b11010000:
    case 0b11011000: //RET cond | conditional return
        if (is_cc(reg, (opcode>>3)&3)) {
            offset_pc = 0;
            memcpy(&((*reg).PC), ram+(*reg).SP, 2);
            (*reg).SP+=2;
        } else {
            offset_pc = 1;
        }
        break;
    case 0b11001001: //RET | return
        offset_pc = 0;
        memcpy(&((*reg).PC), ram+(*reg).SP, 2);
        (*reg).SP+=2;
        break;
    case 0b11011001: //RETI | return and enable IME
        offset_pc = 0;
        memcpy(&((*reg).PC), ram+(*reg).SP, 2);
        (*reg).SP+=2;
        set_ime(reg, 1);
        break;
    case 0b11000010:
    case 0b11001010:
    case 0b11010010:
    case 0b11011010: //JP cond, imm16 | conditional jump to imm16
        if (is_cc(reg, (opcode>>3)&3)) {
            offset_pc = 0;
            memcpy(&imm16, ram+(*reg).PC+1, 2);
            set_r16(reg, R16PC, imm16);
        } else {
            offset_pc = 3;
        }
        break;
    case 0b11000011: //JP, imm16 | jump to imm16
        offset_pc = 0;
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        set_r16(reg, R16PC, imm16);
        break;
    case 0b11101001: //JP, HL | jump to address in HL
        offset_pc = 0;
        set_r16(reg, R16PC, get_r16(reg, R16HL));
        break;
    case 0b11000100:
    case 0b11001100:
    case 0b11010100:
    case 0b11011100: //CALL cond, imm16 | conditional call to imm16
        if (is_cc(reg, (opcode>>3)&3)) {
            offset_pc = 0;
            memcpy(&imm16, ram+(*reg).PC+1, 2);
            (*reg).SP-=2;
            (*reg).PC+=3;
            memcpy(ram+(*reg).SP, &((*reg).PC), 2);
            set_r16(reg, R16PC, get_r16(reg, imm16));
        } else {
            offset_pc = 3;
        }
        break;
    case 0b11001101: //CALL, imm16 | call to imm16
        offset_pc = 0;
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        (*reg).SP-=2;
        (*reg).PC+=3;
        memcpy(ram+(*reg).SP, &((*reg).PC), 2);
        set_r16(reg, R16PC, imm16);
        break;
    case 0b11000111:
    case 0b11001111:
    case 0b11010111:
    case 0b11011111:
    case 0b11100111:
    case 0b11101111:
    case 0b11110111:
    case 0b11111111: //RST vec | CALL to address vec*8
        offset_pc = 0;
        working16bit = ((opcode>>2)&7)*8;
        (*reg).SP-=2;
        (*reg).PC+=1;
        memcpy(ram+(*reg).SP, &((*reg).PC), 2);
        set_r16(reg, R16PC, working16bit);
        break;

    default:;
        bool workingflag;
        char buf[64];
        sprintf(buf, "Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
    }
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

    *(ram+(reg).PC) = 0b11111111;

    print_registers(&reg);
    for (int i=0; i<10; i++) {
        printf("\n");
        run_instruction(&rom, ram, &reg);
        print_registers(&reg);
    }

    return 0;
}