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
extern uint16_t system_counter;
bool do_ei_set = 0; //extern
uint8_t haltmode = 0; //extern
void (*scheduled_instructions[10])(uint8_t); //extern
uint8_t num_scheduled_instructions = 0; //return

static uint8_t r8 = 0; // internal current 8-bit register
static uint8_t r16 = 0; // internal current 16-bit register
static uint8_t Z = 0; // internal working byte
static uint8_t W = 0; // internal working byte. Typically the high byte in a 16-bit register
static uint16_t addr = 0; // internal working mem addr
static bool carry = 0; // internal carry bit


static void machine_nop(void) {
    /* do nothing */
    reg.PC++;
}


static void machine_load_r8_r8(void) {
    /* load into register r8 from register r8 */
    uint8_t opcode = read_byte(reg.PC);
    set_r8((opcode>>3)&7, get_r8(opcode&7));
    reg.PC++;
}


static void machine_load_Z_imm8(void) {
    /* load into Z from next byte */
    reg.PC++;
    Z = read_byte(reg.PC);
}


static void machine_load_W_imm8(void) {
    /* load into W from next byte */
    reg.PC++;
    W = read_byte(reg.PC);
}


static void machine_load_r8_Z(void) {
    /* load into register r8 from Z */
    set_r8(r8, Z);
    reg.PC++;
}


static void machine_load_Z_r8(void) {
    /*load into Z from register r8 */
    Z = get_r8(r8);
}


static void machine_load_Z_addr(void) {
    /* load into Z from addr */
    Z = read_byte(addr);
    reg.PC++;
}


static void machine_load_addr_Z(void) {
    /* load into addr from Z */
    write_byte(addr, Z);
    reg.PC++;
}


static void machine_load_addr_low_imm8(void) {
    /* load into addr's low byte from imm8 */
    reg.PC++;
    addr &= 0xF0;
    addr = addr | read_byte(reg.PC);

}


static void machine_load_addr_high_imm8(void) {
    /* load into addr's high byte from imm8 */
    reg.PC++;
    addr &= 0x0F;
    addr = addr | (read_byte(reg.PC)<<8);

}


static void machine_load_r16_WZ(void) {
    /* load into r16 from addr */
    set_r16(r16, (W<<8) + Z);
    reg.PC++;
}


static void machine_load_addr_r16_high(void) {
    /* load into byte pointed to by addr from r16's high byte */
    write_byte(addr, get_r16(r16)>>8);
}


static void machine_load_addr_r16_low(void) {
    /* load into byte pointed to by addr from r16's low byte */
    write_byte(addr, get_r16(r16)&0xF);
    reg.PC++;
}


static void machine_load_sp_hl(void) {
    /* load into sp from hl */
    reg.SP = reg.HL;
}


static void machine_push_r16_high_dec_sp(void) {
    /* push r16 high to stack, then dec sp */
    write_byte(reg.SP, get_r16(r16)>>8);
    reg.SP--;
}


static void machine_push_r16_low(void) {
    /* push r16 low to stack*/
    write_byte(reg.SP, get_r16(r16)>>8);
}


static void machine_pop_Z_dec_sp(void) {
    /* pop into Z from stack, then inc sp */
    Z = read_byte(reg.SP);
    reg.SP++;
}


static void machine_pop_W_dec_sp(void) {
    /* pop into W from stack, then inc sp */
    W = read_byte(reg.SP);
    reg.SP++;
}


static void machine_inc_addr(void) {
    /* increment addr without setting flags */
    addr++;
}


static void machine_inc_r8(void) {
    /* increment r8 */
    set_r8(r8, get_r8(r8)+1);
    set_flag(NFLAG, 0);
    set_flag(ZFLAG, get_r8(r8)==0);
    set_flag(HFLAG, (get_r8(r8)&15)==0);
    reg.PC++;
}


static void machine_inc_r16(void) {
    /* increment r18 */
    set_r16(r16, get_r16(r16)+1);
}

static void machine_inc_Z(void) {
    /* increment Z */
    Z++;
    set_flag(NFLAG, 0);
    set_flag(ZFLAG, Z==0);
    set_flag(HFLAG, (Z&15) == 0);
}


static void machine_dec_sp(void) {
    /* decrement sp without setting flags */
    reg.SP--;
}


static void machine_dec_r8(void) {
    /* decrement r8 */
    set_r8(r8, get_r8(r8)-1);
    set_flag(NFLAG, 1);
    set_flag(ZFLAG, get_r8(r8)==0);
    set_flag(HFLAG, (get_r8(r8)&15)==15);
    reg.PC++;
}


static void machine_dec_r16(void) {
    /* decrement r18 */
    set_r16(r16, get_r16(r16)-1);
}


static void machine_dec_Z(void) {
    /* decrement Z */
    Z--;
    set_flag(NFLAG, 1);
    set_flag(ZFLAG, Z==0);
    set_flag(HFLAG, (Z&15)==15);
}


static void machine_add_l_spl_Z(void) {
    /* add signed Z to sp low and save to l */
    uint16_t working16bit = reg.SP;
    working16bit += (int8_t)Z;
    set_flag(ZFLAG, 0);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, (working16bit&255)<Z);
    set_flag(HFLAG, (working16bit&15)<(Z&15));
    set_r8(R8L, working16bit&0xFF);
    W = working16bit>>8;
}


static void machine_add_h_sph_Z(void) {
    /* add overflow of Z to sp high and save to h */
    set_r8(R8H, W);
    reg.PC++;
}


static void machine_add_Z_spl_Z(void) {
    /* add signed Z to sp low and save to Z */
    uint16_t working16bit = reg.SP&0xFF;
    carry = (int8_t)Z>0; // set if Z is positive
    working16bit += (int8_t)Z;
    set_flag(ZFLAG, 0);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, (working16bit&255)<Z);
    set_flag(HFLAG, (working16bit&15)<(Z&15));
    Z = working16bit&0xFF;
}


static void machine_add_W_sph_c(void) {
    /* add or subtract C flag from SP high and save to W */
    if (get_flag(CFLAG)) {
        if (carry) {
            W = (reg.SP>>8) + 1;
        } else {
            W = (reg.SP>>8) - 1;
        }
    } else {
        W = reg.SP>>8;
    }
}


static void machine_add_l_r16l(void) {
    /* add l to r16 low and save to l */
    uint8_t byte = get_r16(r16)&0xFF;
    set_r8(R8L, get_r8(R8L) + byte);
    carry = byte > get_r8(R8L);
}

static void machine_add_h_r16h(void) {
    /* add h and the carry from r16 low to r16 high and save to h */
    uint8_t byte = get_r16(r16)>>8;
    uint16_t word = get_r8(R8H) + byte + carry;
    byte = (get_r8(R8H)&15) + (byte&15) + carry;
    set_r8(R8H, (uint8_t)word);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, word>>8>0);
    set_flag(HFLAG, byte>>4>0);
    reg.PC++;
}


static void machine_add_Z(void) {
    /* add Z and carry to A, store result to A */
    uint16_t word = get_r8(R8A) + Z + carry;
    uint8_t byte = (get_r8(R8A)&15) + (Z&15) + carry;
    set_r8(R8A, (uint8_t)word);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, word>>8>0);
    set_flag(HFLAG, byte>>4>0);
    reg.PC++;
}


static void machine_sub_Z(void) {
    /* subtract Z and carry from A, store result to A */
    uint16_t word = get_r8(R8A) - Z - carry;
    uint8_t byte = (get_r8(R8A)&15) - (Z&15) - carry;
    set_r8(R8A, (uint8_t)word);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 1);
    set_flag(CFLAG, word>>8>0);
    set_flag(HFLAG, byte>>4>0);
    reg.PC++;
}


static void machine_cmp_Z(void) {
    /* subtract Z from A, do not store result */
    uint16_t word = get_r8(R8A) - Z;
    uint8_t byte = (get_r8(R8A)&15) - (Z&15);
    set_flag(ZFLAG, ((uint8_t)word)==0);
    set_flag(NFLAG, 1);
    set_flag(CFLAG, word>>8>0);
    set_flag(HFLAG, byte>>4>0);
    reg.PC++;
}


static void machine_and_Z(void) {
    /* logical and A and Z, store result to A */
    set(R8A, get_r8(R8A) & Z);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 0);
    set_flag(HFLAG, 1);
    set_flag(CFLAG, 0);
    reg.PC++;
}


static void machine_or_Z(void) {
    /* logical or A and Z, store result to A */
    set(R8A, get_r8(R8A) | Z);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(CFLAG, 0);
    reg.PC++;
}


static void machine_xor_Z(void) {
    /* logical xor A and Z, store result to A */
    set(R8A, get_r8(R8A) ^ Z);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(CFLAG, 0);
    reg.PC++;
}


static void machine_ccf(void) {
    /* complement C flag and clear N and H */
    set_flag(NFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(CFLAG, !get_flag(CFLAG));
    reg.PC++;
}


static void machine_scf(void) {
    /* set C flag and clear N and H */
    set_flag(NFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(CFLAG, 1);
    reg.PC++;
}


static void machine_daa(void) {
    /* run the decimal adjust accumulator */
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
    reg.PC++;
}


static void machine_cpl(void) {
    /* complement A and set flags */
    set_r8(R8A, ~get_r8(R8A));
    set_flag(NFLAG, 1);
    set_flag(HFLAG, 1);
    reg.PC++;
}


static void machine_rlc_r8(void) {
    /* rotate left circular r8 */
    set_flag(CFLAG, get_r8(r8)>>7);
    W = (get_r8(r8) << 1) + get_flag(CFLAG);
    set_r8(r8, W);
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rlc_Z(void) {
    /* rotate left circular Z */
    set_flag(CFLAG, Z>>7);
    W = (Z << 1) + get_flag(CFLAG);
    Z = W;
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_rrc_r8(void) {
    /* rotate right circular r8 */
    set_flag(CFLAG, get_r8(r8)&1);
    W = (get_r8(r8) >> 1) + (get_flag(CFLAG)<<7);
    set_r8(r8, W);
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rrc_Z(void) {
    /* rotate right circular Z */
    set_flag(CFLAG, Z&1);
    W = (Z >> 1) + (get_flag(CFLAG)<<7);
    Z= W;
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_rl_r8(void) {
    /* rotate left r8*/
    carry = get_flag(CFLAG);
    set_flag(CFLAG, get_r8(r8)>>7);
    W = (get_r8(r8) << 1) + carry;
    set_r8(r8, W);
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rl_Z(void) {
    /* rotate left Z*/
    carry = get_flag(CFLAG);
    set_flag(CFLAG, Z>>7);
    W = (Z << 1) + carry;
    Z = W;
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_rr_r8(void) {
    /* rotate right r8 */
    carry = get_flag(CFLAG);
    set_flag(CFLAG, get_r8(r8)&1);
    W = (get_r8(r8) >> 1) + (carry<<7);
    set_r8(r8, W);
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rr_Z(void) {
    /* rotate right Z */
    carry = get_flag(CFLAG);
    set_flag(CFLAG, Z&1);
    W = (Z >> 1) + (carry<<7);
    Z = W;
    set_flag(ZFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_sla_r8(void) {
    /* shift left arithmetic r8 */
    set_flag(CFLAG, get_r8(r8) >> 7);
    set_r8(r8, get_r8(r8)<<1);
    set_flag(ZFLAG, get_r8(r8)==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_sla_Z(void) {
    /* shift left arithmetic Z */
    set_flag(CFLAG, Z >> 7);
    Z = Z<<1;
    set_flag(ZFLAG, Z==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_sra_r8(void) {
    /* shift right arithmetic r8 */
    set_flag(CFLAG, get_r8(r8)&1);
    set_r8(r8, (get_r8(r8)>>1) + (get_r8(r8)&128));
    set_flag(ZFLAG, get_r8(r8)==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_sra_Z(void) {
    /* shift right arithmetic Z */
    set_flag(CFLAG, Z&1);
    Z = (Z>>1) + (Z&128);
    set_flag(ZFLAG, Z==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_swap_r8(void) {
    /* swap nybbles in r8 */
    W = get_r8(r8) << 4;
    set_r8(r8, W + (get_r8(r8) >> 4));
    set_flag(ZFLAG, get_r8(r8)==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, 0);
    reg.PC++;
}


static void machine_swap_Z(void) {
    /* swap nybbles in Z */
    W = Z << 4;
    Z = W + (Z >> 4);
    set_flag(ZFLAG, Z==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, 0);
}


static void machine_srl_r8(void) {
    /* shift right logically r8 */
    set_flag(CFLAG, get_r8(r8)&1);
    set_r8(r8, get_r8(r8)>>1);
    set_flag(ZFLAG, get_r8(r8)==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_srl_Z(void) {
    /* shift right logically Z */
    set_flag(CFLAG, Z&1);
    Z = Z>>1;
    set_flag(ZFLAG, Z==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}





/*************************************************************************************************************************************************************/

static void instr_nop(void) {
    /* do nothing */
    scheduled_instructions[0] = &machine_nop;
    num_scheduled_instructions = 1;
}


static void instr_ld_r8_r8(void) {
    /* load into register r8 from register r */
    scheduled_instructions[0] = &machine_load_r8_r8;
    num_scheduled_instructions = 1;
}


static void instr_ld_r8_imm8(void) {
    /* load into register r8 from imm8 */
    r8 = (read_byte(reg.PC)>>3)&7;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_r8_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_r8_hl(void) {
    /* load into register r8 from byte pointed to by hl */
    r8 = (read_byte(reg.PC)>>3)&7;
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_load_r8_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_hl_r8(void) {
    /* load into byte pointed to by hl from register r8 */
    r8 = (read_byte(reg.PC)>>3)&7;
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_r8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_hl_n(void) {
    /* load into byte pointed to by hl */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    scheduled_instructions[1] = &machine_nop;
    num_scheduled_instructions = 3;
}


static void instr_ld_A_r16(void) {
    /* load into A from byte pointed to by r16 */
    addr = get_r16((read_byte(reg.PC)>>4)&3);
    r8 = R8A;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_load_r8_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_r16_A(void) {
    /* load into byte pointed to by r16 from A */
    addr = get_r16((read_byte(reg.PC)>>4)&3);
    r8 = R8A;
    scheduled_instructions[0] = &machine_load_Z_r8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_a_imm16(void) {
    /* load into A from the byte pointed to by imm16 */
    r8 = R8A;
    scheduled_instructions[0] = &machine_load_addr_low_imm8;
    scheduled_instructions[1] = &machine_load_addr_high_imm8;
    scheduled_instructions[2] = &machine_load_Z_addr;
    scheduled_instructions[3] = &machine_load_r8_Z;
    num_scheduled_instructions = 4;
}


static void instr_ld_imm16_a(void) {
    /* load into the byte pointed to by imm16 from A */
    r8 = R8A;
    scheduled_instructions[0] = &machine_load_addr_low_imm8;
    scheduled_instructions[1] = &machine_load_addr_high_imm8;
    scheduled_instructions[2] = &machine_load_Z_r8;
    scheduled_instructions[3] = &machine_load_addr_Z;
    num_scheduled_instructions = 4;
}


static void instr_ldh_a_c(void) {
    /* load into A from the byte in 0xFF00 + C */
    r8 = R8A;
    addr = 0xFF + get_r8(R8C);
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_load_r8_Z;
    num_scheduled_instructions = 2;
}


static void instr_ldh_c_a(void) {
    /* load into the byte in 0xFF00 + C from register A */
    r8 = R8A;
    addr = 0xFF + get_r8(R8C);
    scheduled_instructions[0] = &machine_load_Z_r8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    num_scheduled_instructions = 2;
}


static void instr_ldh_a_imm8(void) {
    /* load into A from the byte in 0xFF00 + imm8 */
    r8 = R8A;
    addr = 0xFF00;
    scheduled_instructions[0] = &machine_load_addr_low_imm8;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_load_r8_Z;
    num_scheduled_instructions = 3;
}


static void instr_ldh_imm8_a(void) {
    /* load into the byte in 0xFF00 + imm8 from A */
    r8 = R8A;
    addr = 0xFF00;
    scheduled_instructions[0] = &machine_load_addr_low_imm8;
    scheduled_instructions[1] = &machine_load_Z_r8;
    scheduled_instructions[2] = &machine_load_addr_Z;
    num_scheduled_instructions = 3;
}


static void instr_ld_a_hl_minus(void) {
    /* load into A from the byte pointed to by HL, and decrement HL */
    r8 = R8A;
    addr = reg.HL;
    reg.HL--;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_load_r8_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_hl_a_minus(void) {
    /* load into the byte pointed to by HL from A, and decrement HL */
    r8 = R8A;
    addr = reg.HL;
    reg.HL--;
    scheduled_instructions[0] = &machine_load_Z_r8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_a_hl_plus(void) {
    /* load into A from the byte pointed to by HL, and increment HL */
    r8 = R8A;
    addr = reg.HL;
    reg.HL++;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_load_r8_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_hl_a_plus(void) {
    /* load into the byte pointed to by HL from A, and increment HL */
    r8 = R8A;
    addr = reg.HL;
    reg.HL++;
    scheduled_instructions[0] = &machine_load_Z_r8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_r16_imm16(void) {
    /* load into r16 from imm16 */
    r16 = (read_byte(reg.PC)>>4)&3;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_W_imm8;
    scheduled_instructions[2] = &machine_load_r16_WZ;
    num_scheduled_instructions = 3;
}


static void instr_ld_imm16_sp(void) {
    /* load into byte pointed to by imm16 from byte pointed to by SP */
    r16 = R16SP;
    scheduled_instructions[0] = &machine_load_addr_low_imm8;
    scheduled_instructions[1] = &machine_load_addr_high_imm8;
    scheduled_instructions[2] = &machine_load_addr_r16_high;
    scheduled_instructions[3] = &machine_inc_addr;
    scheduled_instructions[4] = &machine_load_addr_r16_low;
    num_scheduled_instructions = 5;
}


static void instr_ld_sp_hl(void) {
    /* load into sp from hl */
    scheduled_instructions[0] = &machine_load_sp_hl;
    scheduled_instructions[1] = machine_nop;
    num_scheduled_instructions = 2;
}


static void instr_push_r16(void) {
    /* push r16 onto stack */
    r16 = (read_byte(reg.PC)>>4)&3;
    scheduled_instructions[0] = &machine_dec_sp;
    scheduled_instructions[1] = &machine_push_r16_high_dec_sp;
    scheduled_instructions[2] = &machine_push_r16_low;
    scheduled_instructions[3] = &machine_nop;
    num_scheduled_instructions = 4;
}


static void instr_pop_r16(void) {
    /* pop r16 from stack */
    r16 = (read_byte(reg.PC)>>4)&3;
    scheduled_instructions[0] = &machine_pop_Z_dec_sp;
    scheduled_instructions[1] = &machine_pop_W_dec_sp;
    scheduled_instructions[2] = &machine_load_r16_WZ;
    num_scheduled_instructions = 3;
}


static void instr_ld_hl_sp_e(void) {
    /* load into hl sum of sp and signed byte e from imm8 */
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_add_l_spl_Z;
    scheduled_instructions[2] = &machine_add_h_sph_Z;
    num_scheduled_instructions = 3;
}


static void instr_add_a_r8(void) {
    /* add A to r8 and store result in A */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    carry = 0;
    scheduled_instructions[0] = &machine_add_Z;
    num_scheduled_instructions = 1;
}


static void instr_add_a_hl(void) {
    /* add A to byte pointed to by hl and store result in A */
    addr = reg.HL;
    carry = 0;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_add_a_imm8(void) {
    /* add A to imm8 and store result in A */
    carry = 0;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_adc_a_r8(void) {
    /* add A to r8 plus carry and and store result in A */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    carry = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_add_Z;
    num_scheduled_instructions = 1;
}


static void instr_adc_a_hl(void) {
    /* add A to byte pointed to by hl plus carry and store result in A */
    addr = reg.HL;
    carry = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_adc_a_imm8(void) {
    /* add A to imm8 plus carry and store result in A */
    carry = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_sub_a_r8(void) {
    /* subtract A and r8 and store result in A */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    carry = 0;
    scheduled_instructions[0] = &machine_sub_Z;
    num_scheduled_instructions = 1;
}


static void instr_sub_a_hl(void) {
    /* subtract A and byte pointed to by hl and store result in A */
    addr = reg.HL;
    carry = 0;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_sub_a_imm8(void) {
    /* subtract A and imm8 and store result in A */
    carry = 0;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_sbc_a_r8(void) {
    /* subtract A and r8 plus carry and and store result in A */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    carry = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_sub_Z;
    num_scheduled_instructions = 1;
}


static void instr_sbc_a_hl(void) {
    /* subtract A and byte pointed to by hl plus carry and store result in A */
    addr = reg.HL;
    carry = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_sbc_a_imm8(void) {
    /* subtract A and imm8 plus carry and store result in A */
    carry = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_cmp_a_r8(void) {
    /* subtract A and r8 but do not store result */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    scheduled_instructions[0] = &machine_cmp_Z;
    num_scheduled_instructions = 1;
}


static void instr_cmp_a_hl(void) {
    /* subtract A and byte pointed to by hl but do not store result */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_cmp_Z;
    num_scheduled_instructions = 2;
}


static void instr_cmp_a_imm8(void) {
    /* subtract A and imm8 but do not store result */
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_cmp_Z;
    num_scheduled_instructions = 2;
}


static void instr_inc_r8(void) {
    /* increment r8 */
    r8 = (read_byte(reg.PC)>>3)&7;
    scheduled_instructions[0] = &machine_inc_r8;
    num_scheduled_instructions = 1;
}


static void instr_inc_hl(void) {
    /* increment byte pointed to by hl */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_inc_Z;
    scheduled_instructions[2] = &machine_load_addr_Z;
    num_scheduled_instructions = 3;
}


static void instr_dec_r8(void) {
    /* decrement r8 */
    r8 = (read_byte(reg.PC)>>3)&7;
    scheduled_instructions[0] = &machine_dec_r8;
    num_scheduled_instructions = 1;
}


static void instr_dec_hl(void) {
    /* decrement byte pointed to by hl */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_dec_Z;
    scheduled_instructions[2] = &machine_load_addr_Z;
    num_scheduled_instructions = 3;
}


static void instr_and_a_r8(void) {
    /* logical and A and r8 and store result in A */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    scheduled_instructions[0] = &machine_and_Z;
    num_scheduled_instructions = 1;
}


static void instr_and_a_hl(void) {
    /* logical and A and byte pointed to by hl and store result in A */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_and_Z;
    num_scheduled_instructions = 2;
}


static void instr_and_a_imm8(void) {
    /* logical and A and imm8 and store result in A */
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_and_Z;
    num_scheduled_instructions = 2;
}


static void instr_or_a_r8(void) {
    /* logical or A and r8 and store result in A */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    scheduled_instructions[0] = &machine_or_Z;
    num_scheduled_instructions = 1;
}


static void instr_or_a_hl(void) {
    /* logical or A and byte pointed to by hl and store result in A */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_or_Z;
    num_scheduled_instructions = 2;
}


static void instr_or_a_imm8(void) {
    /* logical or A and imm8 and store result in A */
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_or_Z;
    num_scheduled_instructions = 2;
}


static void instr_xor_a_r8(void) {
    /* logical xor A and r8 and store result in A */
    Z = get_r8((read_byte(reg.PC)>>3)&7);
    scheduled_instructions[0] = &machine_xor_Z;
    num_scheduled_instructions = 1;
}


static void instr_xor_a_hl(void) {
    /* logical xor A and byte pointed to by hl and store result in A */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_xor_Z;
    num_scheduled_instructions = 2;
}


static void instr_xor_a_imm8(void) {
    /* logical xor A and imm8 and store result in A */
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_xor_Z;
    num_scheduled_instructions = 2;
}


static void instr_ccf(void) {
    /* complement C flag and clear N and H */
    scheduled_instructions[0] = &machine_ccf;
    num_scheduled_instructions = 1;
}


static void instr_ccf(void) {
    /* set C flag and clear N and H */
    scheduled_instructions[0] = &machine_scf;
    num_scheduled_instructions = 1;
}


static void instr_daa(void) {
    /* run the decimal adjust accumulator */
    scheduled_instructions[0] = &machine_daa;
    num_scheduled_instructions = 1;
}


static void instr_cpl(void) {
    /* complement A */
    scheduled_instructions[0] = &machine_cpl;
    num_scheduled_instructions = 1;
}


static void instr_inc_r16(void) {
    /* increment r16 */
    r16 = (read_byte(reg.PC)>>4)&3;
    scheduled_instructions[0] = &machine_inc_r16;
    scheduled_instructions[1] = &machine_nop;
    num_scheduled_instructions = 2;

}


static void instr_dec_r16(void) {
    /* decrement r16 */
    r16 = (read_byte(reg.PC)>>4)&3;
    scheduled_instructions[0] = &machine_dec_r16;
    scheduled_instructions[1] = &machine_nop;
    num_scheduled_instructions = 2;

}


static void instr_add_hl_r16(void) {
    /* add r16 to hl, store in hl */
    r16 = (read_byte(reg.PC)>>4)&3;
    scheduled_instructions[0] = &machine_add_l_r16l;
    scheduled_instructions[1] = &machine_add_h_r16h;
    num_scheduled_instructions = 2;
}


static void instr_add_sp_r8(void) {
    /* add signed r8 to sp, store in sp */
    r16 = R16SP;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_add_Z_spl_Z;
    scheduled_instructions[2] = &machine_add_W_sph_c;
    scheduled_instructions[3] = &machine_load_r16_WZ;
    num_scheduled_instructions = 4;
}


static void instr_rlca(void) {
    /* rotate left circular accumulator */
    r8 = R8A;
    scheduled_instructions[0] = &machine_rlc_r8;
    num_scheduled_instructions = 1;
}


static void instr_rrca(void) {
    /* rotate right circular accumulator */
    r8 = R8A;
    scheduled_instructions[0] = &machine_rrc_r8;
    num_scheduled_instructions = 1;
}


static void instr_rla(void) {
    /* rotate left accumulator */
    r8 = R8A;
    scheduled_instructions[0] = &machine_rl_r8;
    num_scheduled_instructions = 1;
}


static void instr_rra(void) {
    /* rotate right accumulator */
    r8 = R8A;
    scheduled_instructions[0] = &machine_rr_r8;
    num_scheduled_instructions = 1;
}


// static void instr_(void) {
//     scheduled_instructions[] = &machine_;
//     num_scheduled_instructions = ;
// }















void run_instruction(void) {
    /* read the opcode at the PC and execute an instruction */
    uint8_t opcode = read_byte(get_r16(R16PC));

}