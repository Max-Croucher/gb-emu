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

typedef struct
{
    uint8_t pc_offset;
    uint8_t machine_cycles;
} InstructionCost;


InstructionCost block00(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b00 */
    InstructionCost instruction_cost = {0, 1};
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
        instruction_cost = (InstructionCost){3,3};
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        printf("Got %.4x\n", imm16);
        memcpy(ram+imm16, &(*reg).SP, 2);
        break;
    case 0b00000111: //RLCA | bit-shift rotate register A left, store wraparound into C
        instruction_cost = (InstructionCost){1,1};
        set_flag(reg, CFLAG, get_r8(reg, R8A)>>7);
        working8bit = (get_r8(reg, R8A) << 1) + get_flag(reg, CFLAG);
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00001111: //RRCA | bit-shift rotate register A right, store wraparound into C
        instruction_cost = (InstructionCost){1,1};
        set_flag(reg, CFLAG, get_r8(reg, R8A)&1);
        working8bit = (get_r8(reg, R8A) >> 1) + (get_flag(reg, CFLAG)<<7);
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00010111: //RLA | bit-shift rotate register A left, through C
        instruction_cost = (InstructionCost){1,1};
        workingflag = get_flag(reg, CFLAG);
        set_flag(reg, CFLAG, get_r8(reg, R8A)>>7);
        working8bit = (get_r8(reg, R8A) << 1) + workingflag;
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00011111: //RRA | bit-shift rotate register A right, through C
        instruction_cost = (InstructionCost){1,1};
        workingflag = get_flag(reg, CFLAG);
        set_flag(reg, CFLAG, get_r8(reg, R8A)&1);
        working8bit = (get_r8(reg, R8A) >> 1) + (workingflag<<7);
        set_r8(reg, R8A, working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00100111: //DAA | Decimal adjust accumulator
        instruction_cost = (InstructionCost){1,1};
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
        instruction_cost = (InstructionCost){1,1};
        set_r8(reg, R8A, ~get_r8(reg, R8A));
        set_flag(reg, NFLAG, 1);
        set_flag(reg, HFLAG, 1);
        break;
    case 0b00110111: //SCF | set carry flag
        instruction_cost = (InstructionCost){1,1};
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 1);
        break;
    case 0b00111111: //CCF | complement carry flag
        instruction_cost = (InstructionCost){1,1};
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, !get_flag(reg, CFLAG));
        break;
    case 0b00011000: //jr imm8 | relative jump to SIGNED byte in imm8
        instruction_cost = (InstructionCost){2,3};
        int8_t relative_addr;
        memcpy(&relative_addr, ram+(*reg).PC+1, 1);
        set_r16(reg, R16PC, get_r16(reg, R16PC)+relative_addr);
        break;
    case 0b00100000:
    case 0b00101000:
    case 0b00110000:
    case 0b00111000: //jr cond; imm8 | relative conditional jump to SIGNED byte in imm8
        if (is_cc(reg, (opcode>>3)&3)) {
            instruction_cost = (InstructionCost){2,3};
            int8_t relative_addr;
            memcpy(&relative_addr, ram+(*reg).PC+1, 1);
            set_r16(reg, R16PC, get_r16(reg, R16PC)+relative_addr);
        } else {
            instruction_cost = (InstructionCost){2,2};
        }
        break;
    case 0b00010000: //STOP
        instruction_cost = (InstructionCost){2,0};
        print_error("STOP is not implemented!");
        break;
    default:
        has_finished = 0;
    }
    if (has_finished) return instruction_cost;
    //implicit opcodes, controlled by mask 0b00001111
    switch (opcode & 0xF)
    {
    case 0: //NOP
        instruction_cost = (InstructionCost){1,1};
        break;
    case 1: //LD r16, imm16 | load 16 bit value imm16 into r16
        instruction_cost = (InstructionCost){3,3};
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        set_r16(reg, (opcode>>4)&3, imm16);
        break;
    case 2: //LD [r16mem], A | load contents of r8A to the byte pointed to by [r16mem], conditionally incrementing HL
        instruction_cost = (InstructionCost){1,2};
        if (((opcode>>4)&3) == 2) {
            *(ram+get_r16(reg, R16HL)) = get_r8(reg, R8A);
            set_r16(reg, R16HL, get_r16(reg, R16HL)+1);
        } else if (((opcode>>4)&3) == 3) {
            *(ram+get_r16(reg, R16HL)) = get_r8(reg, R8A);
            set_r16(reg, R16HL, get_r16(reg, R16HL)-1);
        } else {
            *(ram+get_r16(reg, (opcode>>4)&3)) = get_r8(reg, R8A);
        }
        break;
    case 10: //LD A, [r16mem] | load byte pointed to by [r16mem] into A, conditionally incrementing HL
        instruction_cost = (InstructionCost){1,2};
        if (((opcode>>4)&3) == 2) {
            set_r8(reg, R8A, *(ram+get_r16(reg, R16HL)));
            set_r16(reg, R16HL, get_r16(reg, R16HL)+1);
        } else if (((opcode>>4)&3) == 3) {
            set_r8(reg, R8A, *(ram+get_r16(reg, R16HL)));
            set_r16(reg, R16HL, get_r16(reg, R16HL)-1);
        } else {
            set_r8(reg, R8A, *(ram+get_r16(reg, (opcode>>4)&3)));
        }
        break;
    case 3: //INC r16 | increment r16
        instruction_cost = (InstructionCost){1,2};
        set_r16(reg, (opcode>>4)&3, get_r16(reg,(opcode>>4)&3)+1);
        break;
    case 11: //DEC r16 | decrement r16
        instruction_cost = (InstructionCost){1,2};
        set_r16(reg, (opcode>>4)&3, get_r16(reg,(opcode>>4)&3)-1);
        break;
    case 9: //ADD HL, r16 | add value in r16 to HL
        instruction_cost = (InstructionCost){1,2};
        uint16_t old_hl = get_r16(reg, R16HL);
        set_r16(reg, R16HL,old_hl+get_r16(reg, (opcode>>4)&3));
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, get_r16(reg, R16HL)<old_hl);
        set_flag(reg, HFLAG, (old_hl&0x0FFF)+(get_r16(reg, (opcode>>4)&3)&0x0FFF)>=0x1000);
        break;
    case 4:
    case 12: //INC r8 | increment r8
        instruction_cost = (InstructionCost){1,1};
        set_r8(reg, (opcode>>3)&7, get_r8(reg, (opcode>>3)&7)+1);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode>>3)&7)==0);
        set_flag(reg, HFLAG, get_r8(reg, (opcode>>3)&7)==16);
        break;
    case 5:
    case 13: //DEC r8 | decrement r8
        instruction_cost = (InstructionCost){1,1};
        set_r8(reg, (opcode>>3)&7, get_r8(reg, (opcode>>3)&7)-1);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode>>3)&7)==0);
        set_flag(reg, HFLAG, get_r8(reg, (opcode>>3)&7)==15);
        break;
    case 6:
    case 14: //LD r8, imm8 | load value imm8 into r8
        instruction_cost = (InstructionCost){2,2};
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
    return instruction_cost;
}

InstructionCost block01(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b01 */
    InstructionCost instruction_cost = {0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    if (opcode == 0b01110110) { //HALT
        print_error("STOP is not implemented!");
    } else { //LD r8, r8 | load r8 into r8
        instruction_cost = (InstructionCost){1,0};
        set_r8(reg, (opcode>>3)&7, get_r8(reg, opcode&7));
    }

    return instruction_cost;
}

InstructionCost block10(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b10 */
    InstructionCost instruction_cost = {0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    switch ((opcode>>3)&7)
    {
    case 0: //ADD A, r8 | add contents of r8 to A
        instruction_cost = (InstructionCost){1,1};
        set_r8(reg, R8A, get_r8(reg, R8A) + get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, get_r8(reg, R8A)<get_r8(reg, opcode&7));
        set_flag(reg, HFLAG, (get_r8(reg, R8A)&15)<(get_r8(reg, opcode&7)&15));
        break;
    case 1: //ADC A, r8 | add carry and contents of r8 to A
        instruction_cost = (InstructionCost){1,1};
        working16bit = get_r8(reg, R8A) + get_r8(reg, opcode&7) + get_flag(reg, CFLAG);
        working8bit = get_r8(reg, R8A)&15 + (get_r8(reg, opcode&7)&15) + get_flag(reg, CFLAG);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 2: //SUB A, r8 | subtract contents of r8 from A
        instruction_cost = (InstructionCost){1,1};
        working16bit = get_r8(reg, R8A) - get_r8(reg, opcode&7);
        working8bit = (get_r8(reg, R8A)&15) - (get_r8(reg, opcode&7)&15);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 3: //SBC A, r8 | subtract carry and contents of r8 to A
        instruction_cost = (InstructionCost){1,1};
        working16bit = get_r8(reg, R8A) - get_r8(reg, opcode&7) - get_flag(reg, CFLAG);
        working8bit = (get_r8(reg, R8A)&15) - (get_r8(reg, opcode&7)&15) - get_flag(reg, CFLAG);
        set_r8(reg, R8A, (uint8_t)working16bit);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 1);
        set_flag(reg, CFLAG, working16bit>>8>0);
        set_flag(reg, HFLAG, working8bit>>4>0);
        break;
    case 4: //AND A, r8 | logical and between r8 and A
        instruction_cost = (InstructionCost){1,1};
        set_r8(reg, R8A, get_r8(reg, R8A) & get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 1);
        set_flag(reg, CFLAG, 0);
        break;
    case 5: //XOR A, r8 | logical xor between r8 and A
        instruction_cost = (InstructionCost){1,1};
        set_r8(reg, R8A, get_r8(reg, R8A) ^ get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 6: //OR A, r8 | logical or between r8 and A
        instruction_cost = (InstructionCost){1,1};
        set_r8(reg, R8A, get_r8(reg, R8A) | get_r8(reg, opcode&7));
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 7: //CP A, r8 | subtract contents of r8 from A but discard result
        instruction_cost = (InstructionCost){1,1};
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
    return instruction_cost;
}


InstructionCost prefixCB(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute a 0xCB prefixed opcode. IMPORTANT: the offset of the prefix is already accounted for*/
    InstructionCost instruction_cost = {0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;
    switch (opcode>>3)
    {
    /***************************************************************************************************************************************/
    case 0b00000: //RLC r8 | bit-shift rotate register r8 left, store wraparound into C
        instruction_cost = (InstructionCost){1,2};
        set_flag(reg, CFLAG, get_r8(reg, (opcode&7))>>7);
        working8bit = (get_r8(reg, (opcode&7)) << 1) + get_flag(reg, CFLAG);
        set_r8(reg, (opcode&7), working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00001: //RRC r8 | bit-shift rotate register r8 right, store wraparound into C
        instruction_cost = (InstructionCost){1,2};
        set_flag(reg, CFLAG, get_r8(reg, (opcode&7))&1);
        working8bit = (get_r8(reg, (opcode&7)) >> 1) + (get_flag(reg, CFLAG)<<7);
        set_r8(reg, (opcode&7), working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00010: //RL r8 | bit-shift rotate register r8 left, through C
        instruction_cost = (InstructionCost){1,2};
        workingflag = get_flag(reg, CFLAG);
        set_flag(reg, CFLAG, get_r8(reg, (opcode&7))>>7);
        working8bit = (get_r8(reg, (opcode&7)) << 1) + workingflag;
        set_r8(reg, (opcode&7), working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00011: //RR r8 | bit-shift rotate register r8 right, through C
        instruction_cost = (InstructionCost){1,2};
        workingflag = get_flag(reg, CFLAG);
        set_flag(reg, CFLAG, get_r8(reg, (opcode&7))&1);
        working8bit = (get_r8(reg, (opcode&7)) >> 1) + (workingflag<<7);
        set_r8(reg, (opcode&7), working8bit);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00100: //SLA r8 | bit-shit arithmetically register r8 left, into C
        instruction_cost = (InstructionCost){1,2};
        set_flag(reg, CFLAG, get_r8(reg, (opcode&7)) >> 7);
        set_r8(reg, (opcode&7), get_r8(reg, (opcode&7))<<1);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode&7))==0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00101: //SRA r8 | bit-shit arithmetically register r8 right, into C
        instruction_cost = (InstructionCost){1,2};
        set_flag(reg, CFLAG, get_r8(reg, (opcode&7))&1);
        set_r8(reg, (opcode&7), get_r8(reg, (opcode&7))>>1 + get_r8(reg, (opcode&7))&128);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode&7))==0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    case 0b00110: //SWAP r8 | swap upper 4 and lower 4 bits in register r8
        instruction_cost = (InstructionCost){1,2};
        imm8 = get_r8(reg, (opcode&7)) << 4;
        set_r8(reg, (opcode&7), get_r8(reg, imm8 + (opcode&7)) >> 4);
        break;
    case 0b00111: //SRL r8 | bit-shift logically register r8 right, into C
        instruction_cost = (InstructionCost){1,2};
        set_flag(reg, CFLAG, get_r8(reg, (opcode&7))&1);
        set_r8(reg, (opcode&7), get_r8(reg, (opcode&7))>>1);
        set_flag(reg, ZFLAG, get_r8(reg, (opcode&7))==0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, NFLAG, 0);
        break;
    default:;
        bool workingflag;
        char buf[80];
        sprintf(buf, "Unknown 0xCB prefixed opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
    }
    return instruction_cost;
}


InstructionCost block11(gbRom* rom, uint8_t* ram, Registers* reg, uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b11 */
    InstructionCost instruction_cost = {0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    switch (opcode)
    {
    case 0b11000110: //ADD A, imm8 | add contents of imm8 to A
        instruction_cost = (InstructionCost){2,2};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) + imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, CFLAG, get_r8(reg, R8A)<imm8);
        set_flag(reg, HFLAG, (get_r8(reg, R8A)&15)<(imm8&15));
        break;
    case 0b11001110: //ADC A, imm8 | add carry and contents of imm8 to A
        instruction_cost = (InstructionCost){2,2};
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
        instruction_cost = (InstructionCost){2,2};
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
        instruction_cost = (InstructionCost){2,2};
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
        instruction_cost = (InstructionCost){2,2};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) & imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 1);
        set_flag(reg, CFLAG, 0);
        break;
    case 0b11101110: //XOR A, imm8 | logical xor between imm8 and A
        instruction_cost = (InstructionCost){2,2};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) ^ imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 0b11110110: //OR A, imm8 | logical or between imm8 and A
        instruction_cost = (InstructionCost){2,2};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, get_r8(reg, R8A) | imm8);
        set_flag(reg, ZFLAG, get_r8(reg, R8A)==0);
        set_flag(reg, NFLAG, 0);
        set_flag(reg, HFLAG, 0);
        set_flag(reg, CFLAG, 0);
        break;
    case 0b11111110: //CP A, imm8 | subtract contents of imm8 from A but discard result
        instruction_cost = (InstructionCost){2,2};
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
            instruction_cost = (InstructionCost){0,5};
            memcpy(&((*reg).PC), ram+(*reg).SP, 2);
            (*reg).SP+=2;
        } else {
            instruction_cost = (InstructionCost){1,2};
        }
        break;
    case 0b11001001: //RET | return
        instruction_cost = (InstructionCost){0,4};
        memcpy(&((*reg).PC), ram+(*reg).SP, 2);
        (*reg).SP+=2;
        break;
    case 0b11011001: //RETI | return and enable IME
        instruction_cost = (InstructionCost){0,4};
        memcpy(&((*reg).PC), ram+(*reg).SP, 2);
        (*reg).SP+=2;
        set_ime(reg, 1);
        break;
    case 0b11000010:
    case 0b11001010:
    case 0b11010010:
    case 0b11011010: //JP cond, imm16 | conditional jump to imm16
        if (is_cc(reg, (opcode>>3)&3)) {
            instruction_cost = (InstructionCost){0,4};
            memcpy(&imm16, ram+(*reg).PC+1, 2);
            set_r16(reg, R16PC, imm16);
        } else {
            instruction_cost = (InstructionCost){3,3};
        }
        break;
    case 0b11000011: //JP, imm16 | jump to imm16
        instruction_cost = (InstructionCost){0,3};
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        set_r16(reg, R16PC, imm16);
        break;
    case 0b11101001: //JP, HL | jump to address in HL
        instruction_cost = (InstructionCost){0,1};
        set_r16(reg, R16PC, get_r16(reg, R16HL));
        break;
    case 0b11000100:
    case 0b11001100:
    case 0b11010100:
    case 0b11011100: //CALL cond, imm16 | conditional call to imm16
        if (is_cc(reg, (opcode>>3)&3)) {
            instruction_cost = (InstructionCost){0,6};
            memcpy(&imm16, ram+(*reg).PC+1, 2);
            (*reg).SP-=2;
            (*reg).PC+=3;
            memcpy(ram+(*reg).SP, &((*reg).PC), 2);
            set_r16(reg, R16PC, get_r16(reg, imm16));
        } else {
            instruction_cost = (InstructionCost){3,3};
        }
        break;
    case 0b11001101: //CALL, imm16 | call to imm16
        instruction_cost = (InstructionCost){0,6};
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
        instruction_cost = (InstructionCost){0,4};
        working16bit = ((opcode>>2)&7)*8;
        (*reg).SP-=2;
        (*reg).PC+=1;
        memcpy(ram+(*reg).SP, &((*reg).PC), 2);
        set_r16(reg, R16PC, working16bit);
        break;
    case 0b11000001:
    case 0b11010001:
    case 0b11100001:
    case 0b11110001: //POP r16stk | pop register from stack into r16stk
        instruction_cost = (InstructionCost){1,3};
        memcpy(&working16bit, ram+(*reg).SP, 2);
        (*reg).SP+=2;
        switch ((opcode>>4)&3)
        {
        case 0:
            (*reg).BC = working16bit;
            break;
        case 1:
            (*reg).DE = working16bit;
            break;
        case 2:
            (*reg).HL = working16bit;
            break;
        case 3:
            (*reg).AF = working16bit;
            break;
        }
        break;
    case 0b11000101:
    case 0b11010101:
    case 0b11100101:
    case 0b11110101: //PUSH r16stk | push register from r16stk into stack
        instruction_cost = (InstructionCost){1,4};
        switch ((opcode>>4)&3)
        {
        case 0:
            working16bit = (*reg).BC;
            break;
        case 1:
            working16bit = (*reg).DE;
            break;
        case 2:
            working16bit = (*reg).HL;
            break;
        case 3:
            working16bit = (*reg).AF;
            break;
        }
        (*reg).SP-=2;
        memcpy(ram+(*reg).SP, &working16bit, 2);
        break;
    case 0b11001011: // All prefixed 0xCB opcodes
        instruction_cost = prefixCB(rom, ram, reg, *(&((*reg).PC)+1));
        instruction_cost.pc_offset += 1;
        break;
    case 0b11100010: // LDH [c], A | load the byte in A to [0xFF00+C] 
        instruction_cost = (InstructionCost){1,2};
        *(ram+0xFF00+get_r8(reg, R8C)) = get_r8(reg, R8A);
        break;
    case 0b11100000: // LDH [imm8], A | load the byte in A to [0xFF00+imm8] 
        instruction_cost = (InstructionCost){2,3};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        *(ram+0xFF00+imm8) = get_r8(reg, R8A);
        break;
    case 0b11101010: // LD [imm16], A | load the byte in A to [imm16] 
        instruction_cost = (InstructionCost){3,4};
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        *(ram+imm16) = get_r8(reg, R8A);
        break;
    case 0b11110010: // LDH A, [c] | load the byte in [0xFF00+C] to A 
        instruction_cost = (InstructionCost){1,2};
        set_r8(reg, R8A, *(ram+0xFF00+get_r8(reg, R8C)));
        break;
    case 0b11110000: // LDH A, [imm8] | load the byte in [0xFF00+imm8] to A 
        instruction_cost = (InstructionCost){2,3};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        set_r8(reg, R8A, *(ram+0xFF00+imm8));
        break;
    case 0b11111010: // LD A, [imm16] | load the byte in [imm16] to A
        instruction_cost = (InstructionCost){3,4};
        memcpy(&imm16, ram+(*reg).PC+1, 2);
        set_r8(reg, R8A, *(ram+imm16));
        break;
    case 0b11101000: // ADD SP, imm8 | add imm8 to SP
        instruction_cost = (InstructionCost){2,4};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        working16bit = get_r16(reg, R16SP);
        set_r16(reg, R16SP, working16bit + (int8_t)imm8);
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, NFLAG, 0);
        if ((int8_t)imm8 >= 0) {
            set_flag(reg, CFLAG, (get_r16(reg, R16SP)&255)<imm8);
            set_flag(reg, HFLAG, (get_r16(reg, R16SP)&15)<(imm8&15));
        } else {
            working8bit = (imm8&0x8F); // "bitmask" a negative number
            set_flag(reg, CFLAG, (get_r16(reg, R16SP)&255)<working16bit);
            set_flag(reg, HFLAG, (get_r16(reg, R16SP)&15)<((int8_t)working8bit));
        }
        break; 
    case 0b11111000: // LD HL, SP + imm8 | add imm8 to SP and copy result to HL
        instruction_cost = (InstructionCost){2,3};
        memcpy(&imm8, ram+(*reg).PC+1, 1);
        working16bit = get_r16(reg, R16SP);
        set_r16(reg, R16SP, working16bit + (int8_t)imm8);
        set_r16(reg, R16HL, get_r16(reg, R16SP));
        set_flag(reg, ZFLAG, 0);
        set_flag(reg, NFLAG, 0);
        if ((int8_t)imm8 >= 0) {
            set_flag(reg, CFLAG, (get_r16(reg, R16SP)&255)<imm8);
            set_flag(reg, HFLAG, (get_r16(reg, R16SP)&15)<(imm8&15));
        } else {
            working8bit = (imm8&0x8F); // "bitmask" a negative number
            set_flag(reg, CFLAG, (get_r16(reg, R16SP)&255)<working16bit);
            set_flag(reg, HFLAG, (get_r16(reg, R16SP)&15)<((int8_t)working8bit));
        }
        break;
    case 0b11111001: //LD SP, HL | copy HL into SP
        instruction_cost = (InstructionCost){1,2};
        set_r16(reg, R16SP, get_r16(reg, R16HL));
        break;
    case 0b11110011: //DI | disable interrupts
        instruction_cost = (InstructionCost){1,1};
        set_ime(reg, 0);
        break;
    case 0b11111011: //EI | enable interrupts
        instruction_cost = (InstructionCost){1,1};
        set_ime(reg, 1);
        break;
    default:;
        bool workingflag;
        char buf[64];
        sprintf(buf, "Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d",
            (*reg).PC,opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        print_error(buf);
    }
    return instruction_cost;
}


void run_instruction(gbRom* rom, uint8_t* ram, Registers* reg) {
    /* read the opcode at the PC and execute an instruction */
    uint8_t opcode = *(ram+(*reg).PC);
    printf("PC=0x%.4x OPCODE=0x%.2x BITS=",(*reg).PC, opcode);
    for (int8_t i=7; i>=0; i--) {
        printf("%d", (opcode>>i) & 1);
    }
    printf("\n");
    InstructionCost instruction_cost;
    switch (opcode>>6)
    {
    case 0:
        instruction_cost = block00(rom, ram, reg, opcode);
        break;
    case 1:
        instruction_cost = block01(rom, ram, reg, opcode);
        break;
    case 2:
        instruction_cost = block10(rom, ram, reg, opcode);
        break;
    case 3:
        instruction_cost = block11(rom, ram, reg, opcode);
        break;
    }
    (*reg).PC+= instruction_cost.pc_offset;
}

int main(int argc, char *argv[]) {
    gbRom rom = load_rom(argv[1]);
    uint8_t *ram = init_ram(&rom);
    Registers reg = init_registers();


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
    // *(ram+(reg).PC+12) = 0b11000001; //Pop BC
    // *(ram+(reg).PC+13) = 0b11010001; //Pop DE
    // *(ram+(reg).PC+14) = 0b11100001; //Pop HL
    // *(ram+(reg).PC+15) = 0b00000000; //NOP

    print_registers(&reg);
    for (int i=0; i<2; i++) {
        printf("\n");
        run_instruction(&rom, ram, &reg);
        print_registers(&reg);
    }

    return 0;
}