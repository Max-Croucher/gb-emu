/* Source file for opcodes.c, implementing the gameboy's opcodes. Welcome to switch/case hell.
    Author: Max Croucher
    Email: mpccroucher@gmail.com
    May 2025
*/

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "opcodes.h"

extern Registers reg;
extern uint8_t* ram;

InstructionResult block00(uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b00 */
    InstructionResult instruction_result ={0, 0, 0, 1};
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
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+3,3};
        imm16 = read_word(get_r16(R16PC)+1);
        write_word(imm16, reg.SP);
        break;
    case 0b00000111: //RLCA | bit-shift rotate register A left, store wraparound into C
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_flag(CFLAG, get_r8(R8A)>>7);
        working8bit = (get_r8(R8A) << 1) + get_flag(CFLAG);
        set_r8(R8A, working8bit);
        set_flag(ZFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(NFLAG, 0);
        break;
    case 0b00001111: //RRCA | bit-shift rotate register A right, store wraparound into C
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_flag(CFLAG, get_r8(R8A)&1);
        working8bit = (get_r8(R8A) >> 1) + (get_flag(CFLAG)<<7);
        set_r8(R8A, working8bit);
        set_flag(ZFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(NFLAG, 0);
        break;
    case 0b00010111: //RLA | bit-shift rotate register A left, through C
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        workingflag = get_flag(CFLAG);
        set_flag(CFLAG, get_r8(R8A)>>7);
        working8bit = (get_r8(R8A) << 1) + workingflag;
        set_r8(R8A, working8bit);
        set_flag(ZFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(NFLAG, 0);
        break;
    case 0b00011111: //RRA | bit-shift rotate register A right, through C
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        workingflag = get_flag(CFLAG);
        set_flag(CFLAG, get_r8(R8A)&1);
        working8bit = (get_r8(R8A) >> 1) + (workingflag<<7);
        set_r8(R8A, working8bit);
        set_flag(ZFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(NFLAG, 0);
        break;
    case 0b00100111: //DAA | Decimal adjust accumulator | Adjust the decimal-encoded integer stored in A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        uint8_t daa_adj = 0;
        if (get_flag(NFLAG)) {
            if (get_flag(HFLAG)) daa_adj+= 0x06;
            if (get_flag(CFLAG)) daa_adj+= 0x60;
            set_r8(R8A, get_r8(R8A) - daa_adj);
        } else {
            if (get_flag(HFLAG) || (get_r8(R8A)&0x0F)>0x09) daa_adj+= 0x06;
            if (get_flag(CFLAG) || get_r8(R8A)>0x99) {daa_adj+= 0x60; set_flag(CFLAG, 1);}
            set_flag(CFLAG, get_flag(CFLAG) || get_r8(R8A)>0x99);
            set_r8(R8A, get_r8(R8A) + daa_adj);
        }
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(HFLAG, 0);
        break;
    case 0b00101111: //CPL | bitwise not
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_r8(R8A, ~get_r8(R8A));
        set_flag(NFLAG, 1);
        set_flag(HFLAG, 1);
        break;
    case 0b00110111: //SCF | set carry flag
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(CFLAG, 1);
        break;
    case 0b00111111: //CCF | complement carry flag
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(CFLAG, !get_flag(CFLAG));
        break;
    case 0b00011000:; //jr imm8 | relative jump to SIGNED byte in imm8
        int8_t relative_addr;
        relative_addr = read_byte(get_r16(R16PC)+1);
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+relative_addr+2,3};
        break;
    case 0b00100000:
    case 0b00101000:
    case 0b00110000:
    case 0b00111000: //jr cond; imm8 | relative conditional jump to SIGNED byte in imm8
        if (is_cc((opcode>>3)&3)) {
            int8_t relative_addr;
            relative_addr = read_byte(get_r16(R16PC)+1);
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+relative_addr+2,3};
        } else {
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        }
        break;
    case 0b00010000: //STOP
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,0};

        if (*(ram+0xFF00)&0x0F) { //button is being held
            if (*(ram+0xFF0F)&*(ram+0xFFFF)) { // interrupt is pending
                instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2}; // stop is 1-byte, no HALT, no DIV reset
            } else {
                instruction_result = (InstructionResult){0,1,get_r16(R16PC)+2,2}; // stop is 2-byte, HALT mode, no DIV reset
            }
        } else {
            if (*(ram+0xFF0F)&*(ram+0xFFFF)) {
                instruction_result = (InstructionResult){0,2,get_r16(R16PC)+1,2}; // stop is 1-byte, STOP mode, DIV reset
                *(ram+0xFF04) = 0;
            } else {
                instruction_result = (InstructionResult){0,2,get_r16(R16PC)+2,2}; // stop is 2-byte, STOP mode, DIV reset
                *(ram+0xFF04) = 0;
            }
        }
        fprintf(stderr, "WARNING: should STOP be encountered?\n");
        break;
    default:
        has_finished = 0;
    }
    if (has_finished) return instruction_result;
    //implicit opcodes, controlled by mask 0b00001111
    switch (opcode & 0xF)
    {
    case 0: //NOP
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        break;
    case 1: //LD r16, imm16 | load 16 bit value imm16 into r16
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+3,3};
        imm16 = read_word(get_r16(R16PC)+1);
        set_r16((opcode>>4)&3, imm16);
        break;
    case 2: //LD [r16mem], A | load contents of r8A to the byte pointed to by [r16mem], conditionally incrementing HL
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        if (((opcode>>4)&3) == 2) {
            write_byte(get_r16(R16HL), get_r8(R8A));
            set_r16(R16HL, get_r16(R16HL)+1);
        } else if (((opcode>>4)&3) == 3) {
            write_byte(get_r16(R16HL), get_r8(R8A));
            set_r16(R16HL, get_r16(R16HL)-1);
        } else {
            write_byte(get_r16((opcode>>4)&3), get_r8(R8A));
        }
        break;
    case 10: //LD A, [r16mem] | load byte pointed to by [r16mem] into A, conditionally incrementing HL
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        if (((opcode>>4)&3) == 2) {
            set_r8(R8A, read_byte(get_r16(R16HL)));
            set_r16(R16HL, get_r16(R16HL)+1);
        } else if (((opcode>>4)&3) == 3) {
            set_r8(R8A, read_byte(get_r16(R16HL)));
            set_r16(R16HL, get_r16(R16HL)-1);
        } else {
            set_r8(R8A, read_byte(get_r16((opcode>>4)&3)));
        }
        break;
    case 3: //INC r16 | increment r16
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        set_r16((opcode>>4)&3, get_r16((opcode>>4)&3)+1);
        break;
    case 11: //DEC r16 | decrement r16
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        set_r16((opcode>>4)&3, get_r16((opcode>>4)&3)-1);
        break;
    case 9: //ADD HL, r16 | add value in r16 to HL
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        uint16_t old_hl = get_r16(R16HL);
        set_r16(R16HL,old_hl+get_r16((opcode>>4)&3));
        set_flag(NFLAG, 0);
        set_flag(CFLAG, get_r16(R16HL)<old_hl);
        set_flag(HFLAG, (get_r16(R16HL)&4095)<(old_hl&4095));
        break;
    case 4:
    case 12: //INC r8 | increment r8
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_r8((opcode>>3)&7, get_r8((opcode>>3)&7)+1);
        set_flag(NFLAG, 0);
        set_flag(ZFLAG, get_r8((opcode>>3)&7)==0);
        set_flag(HFLAG, (get_r8((opcode>>3)&7)&15) == 0);
        break;
    case 5:
    case 13: //DEC r8 | decrement r8
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        working8bit = get_r8((opcode>>3)&7)-1;
        set_r8((opcode>>3)&7, working8bit);
        set_flag(NFLAG, 1);
        set_flag(ZFLAG, working8bit==0);
        set_flag(HFLAG, (working8bit&15)==15);
        break;
    case 6:
    case 14: //LD r8, imm8 | load value imm8 into r8
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        set_r8((opcode>>3)&7, imm8);
        break;
        
    default:; //Unknown opcode
        fprintf(stderr, "Error: Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d\n",
            get_r16(R16PC),opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        exit(EXIT_FAILURE);
        break;
    }
    return instruction_result;
}


InstructionResult block01(uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b01 */
    InstructionResult instruction_result ={0, 0, 0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    if (opcode == 0b01110110) { //HALT
        instruction_result = (InstructionResult){1,0,get_r16(R16PC)+1,0};
    } else { //LD r8, r8 | load r8 into r8
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,0};
        set_r8((opcode>>3)&7, get_r8(opcode&7));
    }

    return instruction_result;
}


InstructionResult block10(uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b10 */
    InstructionResult instruction_result ={0, 0, 0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    switch ((opcode>>3)&7)
    {
    case 0: //ADD A, r8 | add contents of r8 to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        working8bit = get_r8(opcode&7);
        set_r8(R8A, get_r8(R8A) + working8bit);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(CFLAG, get_r8(R8A)<working8bit);
        set_flag(HFLAG, (get_r8(R8A)&15)<(working8bit&15));
        break;
    case 1: //ADC A, r8 | add carry and contents of r8 to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        working16bit = get_r8(R8A) + get_r8(opcode&7) + get_flag(CFLAG);
        working8bit = (get_r8(R8A)&15) + (get_r8(opcode&7)&15) + get_flag(CFLAG);
        set_r8(R8A, (uint8_t)working16bit);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(CFLAG, working16bit>>8>0);
        set_flag(HFLAG, working8bit>>4>0);
        break;
    case 2: //SUB A, r8 | subtract contents of r8 from A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        working16bit = get_r8(R8A) - get_r8(opcode&7);
        working8bit = (get_r8(R8A)&15) - (get_r8(opcode&7)&15);
        set_r8(R8A, (uint8_t)working16bit);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 1);
        set_flag(CFLAG, working16bit>>8>0);
        set_flag(HFLAG, working8bit>>4>0);
        break;
    case 3: //SBC A, r8 | subtract carry and contents of r8 to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        working16bit = get_r8(R8A) - get_r8(opcode&7) - get_flag(CFLAG);
        working8bit = (get_r8(R8A)&15) - (get_r8(opcode&7)&15) - get_flag(CFLAG);
        set_r8(R8A, (uint8_t)working16bit);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 1);
        set_flag(CFLAG, working16bit>>8>0);
        set_flag(HFLAG, working8bit>>4>0);
        break;
    case 4: //AND A, r8 | logical and between r8 and A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_r8(R8A, get_r8(R8A) & get_r8(opcode&7));
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 1);
        set_flag(CFLAG, 0);
        break;
    case 5: //XOR A, r8 | logical xor between r8 and A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_r8(R8A, get_r8(R8A) ^ get_r8(opcode&7));
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(CFLAG, 0);
        break;
    case 6: //OR A, r8 | logical or between r8 and A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_r8(R8A, get_r8(R8A) | get_r8(opcode&7));
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(CFLAG, 0);
        break;
    case 7: //CP A, r8 | subtract contents of r8 from A but discard result
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        imm8 = get_r8(opcode&7);
        set_flag(ZFLAG, get_r8(R8A)==imm8);
        set_flag(NFLAG, 1);
        set_flag(CFLAG, imm8 > get_r8(R8A));
        set_flag(HFLAG, (imm8&15) > (get_r8(R8A)&15));
        break;
    default:;
        fprintf(stderr, "Error: Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d\n",
            get_r16(R16PC),opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        exit(EXIT_FAILURE);
    }
    return instruction_result;
}


InstructionResult prefixCB(uint8_t opcode) {
    /* execute a 0xCB prefixed opcode. IMPORTANT: the offset of the prefix is already accounted for*/
    InstructionResult instruction_result ={0, 0, 0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;
    switch (opcode>>6)
    {
    case 1: //BIT b3, r8 | set zflag to bit b3 in r8
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,3};
        working8bit = (opcode>>3)&7;
        set_flag(ZFLAG, (get_r8(opcode&7) & (1<<working8bit))==0);
        set_flag(HFLAG, 1);
        set_flag(NFLAG, 0);
        break;
    case 2: //RES b3, r8 | reset bit b3 in r8 to 0
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,4};
        working8bit = (opcode>>3)&7;
        set_r8(opcode&7, get_r8(opcode&7) & ~(1<<working8bit));
        break;
    case 3: //SET b3, r8 | set bit b3 in r8 to 1
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,4};
        working8bit = (opcode>>3)&7;
        set_r8(opcode&7, get_r8(opcode&7) | (1<<working8bit));
        break;
    default:
        switch (opcode>>3)
        {
        case 0b00000: //RLC r8 | bit-shift rotate register r8 left, store wraparound into C
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            set_flag(CFLAG, get_r8((opcode&7))>>7);
            working8bit = (get_r8((opcode&7)) << 1) + get_flag(CFLAG);
            set_r8((opcode&7), working8bit);
            set_flag(ZFLAG, working8bit==0);
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            break;
        case 0b00001: //RRC r8 | bit-shift rotate register r8 right, store wraparound into C
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            set_flag(CFLAG, get_r8((opcode&7))&1);
            working8bit = (get_r8((opcode&7)) >> 1) + (get_flag(CFLAG)<<7);
            set_r8((opcode&7), working8bit);
            set_flag(ZFLAG, working8bit==0);
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            break;
        case 0b00010: //RL r8 | bit-shift rotate register r8 left, through C
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            workingflag = get_flag(CFLAG);
            set_flag(CFLAG, get_r8((opcode&7))>>7);
            working8bit = (get_r8((opcode&7)) << 1) + workingflag;
            set_r8((opcode&7), working8bit);
            set_flag(ZFLAG, working8bit==0);
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            break;
        case 0b00011: //RR r8 | bit-shift rotate register r8 right, through C
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            workingflag = get_flag(CFLAG);
            set_flag(CFLAG, get_r8((opcode&7))&1);
            working8bit = (get_r8((opcode&7)) >> 1) + (workingflag<<7);
            set_r8((opcode&7), working8bit);
            set_flag(ZFLAG, (working8bit==0));
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            break;
        case 0b00100: //SLA r8 | bit-shit arithmetically register r8 left, into C
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            set_flag(CFLAG, get_r8((opcode&7)) >> 7);
            set_r8((opcode&7), get_r8((opcode&7))<<1);
            set_flag(ZFLAG, get_r8((opcode&7))==0);
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            break;
        case 0b00101: //SRA r8 | bit-shit arithmetically register r8 right, into C
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            set_flag(CFLAG, get_r8((opcode&7))&1);
            set_r8((opcode&7), (get_r8((opcode&7))>>1) + (get_r8((opcode&7))&128));
            set_flag(ZFLAG, get_r8((opcode&7))==0);
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            break;
        case 0b00110: //SWAP r8 | swap upper 4 and lower 4 bits in register r8
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            imm8 = get_r8((opcode&7)) << 4;
            set_r8((opcode&7), imm8 + (get_r8((opcode&7)) >> 4));
            set_flag(ZFLAG, get_r8((opcode&7))==0);
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            set_flag(CFLAG, 0);
            break;
        case 0b00111: //SRL r8 | bit-shift logically register r8 right, into C
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
            set_flag(CFLAG, get_r8((opcode&7))&1);
            set_r8((opcode&7), get_r8((opcode&7))>>1);
            set_flag(ZFLAG, get_r8((opcode&7))==0);
            set_flag(HFLAG, 0);
            set_flag(NFLAG, 0);
            break;
        default:;
            fprintf(stderr, "Error: Unknown 0xCB prefixed opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d\n",
                get_r16(R16PC),opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
                (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
            exit(EXIT_FAILURE);
        }
    }
    return instruction_result;
}


InstructionResult block11(uint8_t opcode) {
    /* execute an instruction for an opcode beginning with 0b11 */
    InstructionResult instruction_result ={0, 0, 0, 1};
    uint16_t working16bit;
    uint16_t imm16;
    uint8_t working8bit;
    uint8_t imm8;
    bool workingflag;
    uint8_t has_finished = 1;

    switch (opcode)
    {
    case 0b11000110: //ADD A, imm8 | add contents of imm8 to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        set_r8(R8A, get_r8(R8A) + imm8);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(CFLAG, get_r8(R8A)<imm8);
        set_flag(HFLAG, (get_r8(R8A)&15)<(imm8&15));
        break;
    case 0b11001110: //ADC A, imm8 | add carry and contents of imm8 to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        working16bit = get_r8(R8A) + imm8 + get_flag(CFLAG);
        working8bit = (get_r8(R8A)&15) + (imm8&15) + get_flag(CFLAG);
        set_r8(R8A, (uint8_t)working16bit);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(CFLAG, working16bit>>8>0);
        set_flag(HFLAG, working8bit>>4>0);
        break;
    case 0b11010110: //SUB A, imm8 | subtract contents of imm8 from A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        working16bit = get_r8(R8A) - imm8;
        working8bit = (get_r8(R8A)&15) - (imm8&15);
        set_r8(R8A, (uint8_t)working16bit);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 1);
        set_flag(CFLAG, working16bit>>8>0);
        set_flag(HFLAG, working8bit>>4>0);
        break;
    case 0b11011110: //SBC A, imm8 | subtract carry and contents of imm8 to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        working16bit = get_r8(R8A) - imm8 - get_flag(CFLAG);
        working8bit = (get_r8(R8A)&15) - (imm8&15) - get_flag(CFLAG);
        set_r8(R8A, (uint8_t)working16bit);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 1);
        set_flag(CFLAG, working16bit>>8>0);
        set_flag(HFLAG, working8bit>>4>0);
        break;
    case 0b11100110: //AND A, imm8 | logical and between imm8 and A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        set_r8(R8A, get_r8(R8A) & imm8);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 1);
        set_flag(CFLAG, 0);
        break;
    case 0b11101110: //XOR A, imm8 | logical xor between imm8 and A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        set_r8(R8A, get_r8(R8A) ^ imm8);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(CFLAG, 0);
        break;
    case 0b11110110: //OR A, imm8 | logical or between imm8 and A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        set_r8(R8A, get_r8(R8A) | imm8);
        set_flag(ZFLAG, get_r8(R8A)==0);
        set_flag(NFLAG, 0);
        set_flag(HFLAG, 0);
        set_flag(CFLAG, 0);
        break;
    case 0b11111110: //CP A, imm8 | subtract contents of imm8 from A but discard result
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,2};
        imm8 = read_byte(get_r16(R16PC)+1);
        set_flag(ZFLAG, get_r8(R8A)==imm8);
        set_flag(NFLAG, 1);
        set_flag(CFLAG, imm8 > get_r8(R8A));
        set_flag(HFLAG, (imm8&15) > (get_r8(R8A)&15));
        break;
    case 0b11000000:
    case 0b11001000:
    case 0b11010000:
    case 0b11011000: //RET cond | conditional return
        if (is_cc((opcode>>3)&3)) {
            instruction_result = (InstructionResult){0,0,read_word(reg.SP),5};
            reg.SP+=2;
        } else {
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        }
        break;
    case 0b11001001: //RET | return
        instruction_result = (InstructionResult){0,0,read_word(reg.SP),4};
        reg.SP+=2;
        break;
    case 0b11011001: //RETI | return and enable IME
        instruction_result = (InstructionResult){0,0,read_word(reg.SP),4};
        reg.SP+=2;
        set_ime(1);
        break;
    case 0b11000010:
    case 0b11001010:
    case 0b11010010:
    case 0b11011010: //JP cond, imm16 | conditional jump to imm16
        if (is_cc((opcode>>3)&3)) {
            imm16 = read_word(get_r16(R16PC)+1);
            instruction_result = (InstructionResult){0,0,imm16,4};
        } else {
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+3,3};
        }
        break;
    case 0b11000011: //JP, imm16 | jump to imm16
        imm16 = read_word(get_r16(R16PC)+1);
        instruction_result = (InstructionResult){0,0,imm16,3};
        break;
    case 0b11101001: //JP, HL | jump to address in HL
        instruction_result = (InstructionResult){0,0,get_r16(R16HL),1};
        break;
    case 0b11000100:
    case 0b11001100:
    case 0b11010100:
    case 0b11011100: //CALL cond, imm16 | conditional call to imm16
        if (is_cc((opcode>>3)&3)) {
            imm16 = read_word(get_r16(R16PC)+1);
            reg.SP-=2;
            reg.PC+=3;
            write_word(reg.SP, get_r16(R16PC));
            instruction_result = (InstructionResult){0,0,imm16,6};
        } else {
            instruction_result = (InstructionResult){0,0,get_r16(R16PC)+3,3};
        }
        break;
    case 0b11001101: //CALL, imm16 | call to imm16
        imm16 = read_word(get_r16(R16PC)+1);
        reg.SP-=2;
        reg.PC+=3;
        write_word(reg.SP, get_r16(R16PC));
        instruction_result = (InstructionResult){0,0,imm16,6};
        break;
    case 0b11000111:
    case 0b11001111:
    case 0b11010111:
    case 0b11011111:
    case 0b11100111:
    case 0b11101111:
    case 0b11110111:
    case 0b11111111: //RST vec | CALL to address vec*8
        working16bit = ((opcode>>3)&7)*8;
        instruction_result = (InstructionResult){0,0,working16bit,4};
        reg.SP-=2;
        write_word(reg.SP, get_r16(R16PC)+1);
        break;
    case 0b11000001:
    case 0b11010001:
    case 0b11100001:
    case 0b11110001: //POP r16stk | pop register from stack into r16stk
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,3};
        working16bit = read_word(reg.SP);
        reg.SP+=2;
        switch ((opcode>>4)&3)
        {
        case 0:
            reg.BC = working16bit;
            break;
        case 1:
            reg.DE = working16bit;
            break;
        case 2:
            reg.HL = working16bit;
            break;
        case 3:
            reg.AF = working16bit & 0xFFF0;
            break;
        }
        break;
    case 0b11000101:
    case 0b11010101:
    case 0b11100101:
    case 0b11110101: //PUSH r16stk | push register from r16stk into stack
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,4};
        switch ((opcode>>4)&3)
        {
        case 0:
            working16bit = reg.BC;
            break;
        case 1:
            working16bit = reg.DE;
            break;
        case 2:
            working16bit = reg.HL;
            break;
        case 3:
            working16bit = reg.AF;
            break;
        }
        reg.SP-=2;
        write_word(reg.SP, working16bit);
        break;
    case 0b11001011: // All prefixed 0xCB opcodes
        instruction_result = prefixCB(*(ram+get_r16(R16PC)+1));
        break;
    case 0b11100010: // LDH [c], A | load the byte in A to [0xFF00+C]
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        write_byte(0xFF00+get_r8(R8C), get_r8(R8A));
        break;
    case 0b11100000: // LDH [imm8], A | load the byte in A to [0xFF00+imm8]
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,3};
        imm8 = read_byte(get_r16(R16PC)+1);
        write_byte(0xFF00+imm8, get_r8(R8A));
        break;
    case 0b11101010: // LD [imm16], A | load the byte in A to [imm16]
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+3,4};
        imm16 = read_word(get_r16(R16PC)+1);
        write_byte(imm16, get_r8(R8A));
        break;
    case 0b11110010: // LDH A, [c] | load the byte in [0xFF00+C] to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        set_r8(R8A, read_byte(0xFF00+get_r8(R8C)));
        break;
    case 0b11110000: // LDH A, [imm8] | load the byte in [0xFF00+imm8] to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,3};
        imm8 = read_byte(get_r16(R16PC)+1);
        set_r8(R8A, read_byte(0xFF00+imm8));
        break;
    case 0b11111010: // LD A, [imm16] | load the byte in [imm16] to A
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+3,4};
        imm16 = read_word(get_r16(R16PC)+1);
        set_r8(R8A, read_byte(imm16));
        break;
    case 0b11101000: // ADD SP, imm8 | add SIGNED imm8 to SP
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,4};
        imm8 = read_byte(get_r16(R16PC)+1);
        working16bit = get_r16(R16SP);
        set_r16(R16SP, working16bit + (int8_t)imm8);
        set_flag(ZFLAG, 0);
        set_flag(NFLAG, 0);
        set_flag(CFLAG, (get_r16(R16SP)&255)<imm8);
        set_flag(HFLAG, (get_r16(R16SP)&15)<(imm8&15));

        break; 
    case 0b11111000: // LD HL, SP + imm8 | add imm8 to SP and copy result to HL
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+2,3};
        imm8 = read_byte(get_r16(R16PC)+1);
        working16bit = get_r16(R16SP);
        set_r16(R16HL, working16bit + (int8_t)imm8);
        set_flag(ZFLAG, 0);
        set_flag(NFLAG, 0);
        set_flag(CFLAG, (get_r16(R16HL)&255)<imm8);
        set_flag(HFLAG, (get_r16(R16HL)&15)<(imm8&15));

        break;
    case 0b11111001: //LD SP, HL | copy HL into SP
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,2};
        set_r16(R16SP, get_r16(R16HL));
        break;
    case 0b11110011: //DI | disable interrupts
        instruction_result = (InstructionResult){0,0,get_r16(R16PC)+1,1};
        set_ime(0);
        break;
    case 0b11111011: //EI | enable interrupts
        instruction_result = (InstructionResult){0,1,get_r16(R16PC)+1,1}; // EI is set one cycle later
        break;
    default:;
        fprintf(stderr, "Error: Unknown opcode at PC=0x%.4x OPCODE=0x%.2x BITS=%d%d%d%d%d%d%d%d\n",
            get_r16(R16PC),opcode,(opcode>>7)&1,(opcode>>6)&1,(opcode>>5)&1,(opcode>>4)&1,
            (opcode>>3)&1,(opcode>>2)&1,(opcode>>1)&1,(opcode>>0)&1);
        exit(EXIT_FAILURE);
    }
    return instruction_result;
}


InstructionResult run_instruction() {
    /* read the opcode at the PC and execute an instruction */
    uint8_t opcode = read_byte(get_r16(R16PC));




    InstructionResult instruction_result;
    switch (opcode>>6)
    {
    case 0:
        instruction_result = block00(opcode);
        break;
    case 1:
        instruction_result = block01(opcode);
        break;
    case 2:
        instruction_result = block10(opcode);
        break;
    case 3:
        instruction_result = block11(opcode);
        break;
    }
    return instruction_result;
}