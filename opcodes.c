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
uint8_t do_haltmode = 0; //extern
void (*scheduled_instructions[10])(void); //extern
uint8_t num_scheduled_instructions = 0; //extern
uint8_t current_instruction_count = 0; //extern

static uint8_t r8 = 0; // internal current 8-bit register
static uint8_t r16 = 0; // internal current 16-bit register
static uint8_t Z = 0; // internal working byte
static uint8_t W = 0; // internal working byte. Typically the high byte in a 16-bit register
static uint16_t addr = 0; // internal working mem addr
static bool working_bit = 0; // internal bit. Typically a carry bit or a cc condition
static bool do_zflag = 0; // internal bit. Controls difference in flag behaviour for instructions like RLCA and RLC


static void (*instructions[256])(void) = {
/*  0x00                    0x01                    0x02                    0x03                    0x04                    0x05                    0x06                    0x07                    0x08                    0x09                    0x0A                    0x0B                    0x0C                    0x0D                    0x0E                    0x0F*/
    &instr_nop,             &instr_ld_r16_imm16,    &instr_ld_r16_A,        &instr_inc_r16,         &instr_inc_r8,          &instr_dec_r8,          &instr_ld_r8_imm8,      &instr_rlca,            &instr_ld_imm16_sp,     &instr_add_hl_r16,      &instr_ld_A_r16,        &instr_dec_r16,         &instr_inc_r8,          &instr_dec_r8,          &instr_ld_r8_imm8,      &instr_rrca,        //0x00
    &instr_stop,            &instr_ld_r16_imm16,    &instr_ld_r16_A,        &instr_inc_r16,         &instr_inc_r8,          &instr_dec_r8,          &instr_ld_r8_imm8,      &instr_rla,             &instr_jr_imm8,         &instr_add_hl_r16,      &instr_ld_A_r16,        &instr_dec_r16,         &instr_inc_r8,          &instr_dec_r8,          &instr_ld_r8_imm8,      &instr_rra,         //0x10
    &instr_jr_cc_imm8,      &instr_ld_r16_imm16,    &instr_ld_hl_a_plus,    &instr_inc_r16,         &instr_inc_r8,          &instr_dec_r8,          &instr_ld_r8_imm8,      &instr_daa,             &instr_jr_cc_imm8,      &instr_add_hl_r16,      &instr_ld_a_hl_plus,    &instr_dec_r16,         &instr_inc_r8,          &instr_dec_r8,          &instr_ld_r8_imm8,      &instr_cpl,         //0x20
    &instr_jr_cc_imm8,      &instr_ld_r16_imm16,    &instr_ld_hl_a_minus,   &instr_inc_r16,         &instr_inc_hl,          &instr_dec_hl,          &instr_ld_hl_imm8,      &instr_scf,             &instr_jr_cc_imm8,      &instr_add_hl_r16,      &instr_ld_a_hl_minus,   &instr_dec_r16,         &instr_inc_r8,          &instr_dec_r8,          &instr_ld_r8_imm8,      &instr_ccf,         //0x30
    &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_hl,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_hl,        &instr_ld_r8_r8,    //0x40
    &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_hl,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_hl,        &instr_ld_r8_r8,    //0x50
    &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_hl,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_hl,        &instr_ld_r8_r8,    //0x60
    &instr_ld_hl_r8,        &instr_ld_hl_r8,        &instr_ld_hl_r8,        &instr_ld_hl_r8,        &instr_ld_hl_r8,        &instr_ld_hl_r8,        &instr_halt,            &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_r8,        &instr_ld_r8_hl,        &instr_ld_r8_r8,    //0x70
    &instr_add_a_r8,        &instr_add_a_r8,        &instr_add_a_r8,        &instr_add_a_r8,        &instr_add_a_r8,        &instr_add_a_r8,        &instr_add_a_hl,        &instr_add_a_r8,        &instr_adc_a_r8,        &instr_adc_a_r8,        &instr_adc_a_r8,        &instr_adc_a_r8,        &instr_adc_a_r8,        &instr_adc_a_r8,        &instr_adc_a_hl,        &instr_adc_a_r8,    //0x80
    &instr_sub_a_r8,        &instr_sub_a_r8,        &instr_sub_a_r8,        &instr_sub_a_r8,        &instr_sub_a_r8,        &instr_sub_a_r8,        &instr_sub_a_hl,        &instr_sub_a_r8,        &instr_sbc_a_r8,        &instr_sbc_a_r8,        &instr_sbc_a_r8,        &instr_sbc_a_r8,        &instr_sbc_a_r8,        &instr_sbc_a_r8,        &instr_sbc_a_hl,        &instr_sbc_a_r8,    //0x90
    &instr_and_a_r8,        &instr_and_a_r8,        &instr_and_a_r8,        &instr_and_a_r8,        &instr_and_a_r8,        &instr_and_a_r8,        &instr_and_a_hl,        &instr_and_a_r8,        &instr_xor_a_r8,        &instr_xor_a_r8,        &instr_xor_a_r8,        &instr_xor_a_r8,        &instr_xor_a_r8,        &instr_xor_a_r8,        &instr_xor_a_hl,        &instr_xor_a_r8,    //0xA0
    &instr_or_a_r8,         &instr_or_a_r8,         &instr_or_a_r8,         &instr_or_a_r8,         &instr_or_a_r8,         &instr_or_a_r8,         &instr_or_a_hl,         &instr_or_a_r8,         &instr_cmp_a_r8,        &instr_cmp_a_r8,        &instr_cmp_a_r8,        &instr_cmp_a_r8,        &instr_cmp_a_r8,        &instr_cmp_a_r8,        &instr_cmp_a_hl,        &instr_cmp_a_r8,    //0xB0
    &instr_ret_cc,          &instr_pop_r16,         &instr_jp_cc_imm16,     &instr_jp_imm16,        &instr_call_cc_imm16,   &instr_push_r16,        &instr_add_a_imm8,      &instr_rst,             &instr_ret_cc,          &instr_ret,             &instr_jp_cc_imm16,     &instr_invalid,         &instr_call_cc_imm16,   &instr_call_imm16,      &instr_adc_a_imm8,      &instr_rst,         //0xC0
    &instr_ret_cc,          &instr_pop_r16,         &instr_jp_cc_imm16,     &instr_invalid,         &instr_call_cc_imm16,   &instr_push_r16,        &instr_sub_a_imm8,      &instr_rst,             &instr_ret_cc,          &instr_reti,            &instr_jp_cc_imm16,     &instr_invalid,         &instr_call_cc_imm16,   &instr_invalid,         &instr_sbc_a_imm8,      &instr_rst,         //0xD0
    &instr_ldh_imm8_a,      &instr_pop_r16,         &instr_ldh_c_a,         &instr_invalid,         &instr_invalid,         &instr_push_r16,        &instr_and_a_imm8,      &instr_rst,             &instr_add_sp_r8,       &instr_jp_hl,           &instr_ld_imm16_a,      &instr_invalid,         &instr_invalid,         &instr_invalid,         &instr_xor_a_imm8,      &instr_rst,         //0xE0
    &instr_ldh_a_imm8,      &instr_pop_r16,         &instr_ldh_a_c,         &instr_di,              &instr_invalid,         &instr_push_r16,        &instr_or_a_imm8,       &instr_rst,             &instr_ld_hl_sp_e,      &instr_ld_sp_hl,        &instr_ld_a_imm16,      &instr_ei,              &instr_invalid,         &instr_invalid,         &instr_cmp_a_imm8,      &instr_rst          //0xF0
};

static void (*prefixed_instructions[256])(void) = {
/*  0x00                    0x01                    0x02                    0x03                    0x04                    0x05                    0x06                    0x07                    0x08                    0x09                    0x0A                    0x0B                    0x0C                    0x0D                    0x0E                    0x0F*/
    &instr_rlc_r8,          &instr_rlc_r8,          &instr_rlc_r8,          &instr_rlc_r8,          &instr_rlc_r8,          &instr_rlc_r8,          &instr_rlc_hl,          &instr_rlc_r8,          &instr_rrc_r8,          &instr_rrc_r8,          &instr_rrc_r8,          &instr_rrc_r8,          &instr_rrc_r8,          &instr_rrc_r8,          &instr_rrc_hl,          &instr_rrc_r8,      //0x00
    &instr_rl_r8,           &instr_rl_r8,           &instr_rl_r8,           &instr_rl_r8,           &instr_rl_r8,           &instr_rl_r8,           &instr_rl_hl,           &instr_rl_r8,           &instr_rr_r8,           &instr_rr_r8,           &instr_rr_r8,           &instr_rr_r8,           &instr_rr_r8,           &instr_rr_r8,           &instr_rr_hl,           &instr_rr_r8,       //0x10
    &instr_sla_r8,          &instr_sla_r8,          &instr_sla_r8,          &instr_sla_r8,          &instr_sla_r8,          &instr_sla_r8,          &instr_sla_hl,          &instr_sla_r8,          &instr_sra_r8,          &instr_sra_r8,          &instr_sra_r8,          &instr_sra_r8,          &instr_sra_r8,          &instr_sra_r8,          &instr_sra_hl,          &instr_sra_r8,      //0x20
    &instr_swap_r8,         &instr_swap_r8,         &instr_swap_r8,         &instr_swap_r8,         &instr_swap_r8,         &instr_swap_r8,         &instr_swap_hl,         &instr_swap_r8,         &instr_srl_r8,          &instr_srl_r8,          &instr_srl_r8,          &instr_srl_r8,          &instr_srl_r8,          &instr_srl_r8,          &instr_srl_hl,          &instr_srl_r8,      //0x30
    &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,      //0x40
    &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,      //0x50
    &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,      //0x60
    &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_r8,          &instr_bit_hl,          &instr_bit_r8,      //0x70
    &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,      //0x80
    &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,      //0x90
    &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,      //0xA0
    &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_r8,          &instr_res_hl,          &instr_res_r8,      //0xB0
    &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8,      //0xC0
    &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8,      //0xD0
    &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8,      //0xE0
    &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_r8,          &instr_set_hl,          &instr_set_r8       //0xF0
};



static void machine_nop(void) {
    /* do nothing but increment PC */
    reg.PC++;
}


static void machine_idle(void) {
    /* do nothing and do not increment PC */
}


static void machine_consume_prefix() {
    /* increment PC and then decode registers from opcode */
    reg.PC++;
    r8 = read_byte(reg.PC)&7;
    addr = reg.HL;
    W = (read_byte(reg.PC)>>3)&7;
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
}


static void machine_load_addr_Z(void) {
    /* load into addr from Z */
    write_byte(addr, Z);
    reg.PC++;
}


static void machine_load_addr_low_imm8(void) {
    /* load into addr's low byte from imm8 */
    reg.PC++;
    addr &= 0xFF00;
    addr |= read_byte(reg.PC);
}


static void machine_load_addr_high_imm8(void) {
    /* load into addr's high byte from imm8 */
    reg.PC++;
    addr &= 0x00FF;
    addr |= ((read_byte(reg.PC)<<8));

}


static void machine_load_r16_WZ(void) {
    /* load into r16 from addr */
    reg.PC++; // increment pc before write in case pc is written to
    set_r16(r16, (W<<8) + Z);
}


static void machine_load_addr_r16_high(void) {
    /* load into byte pointed to by addr from r16's high byte */
    write_byte(addr, get_r16(r16)>>8);
    reg.PC++;
}


static void machine_load_addr_r16_low(void) {
    /* load into byte pointed to by addr from r16's low byte */
    write_byte(addr, get_r16(r16)&0x00FF);
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


static void machine_push_r16_low_set_r16_WZ(void) {
    /* push r16 high to stack, then set r16 to WZ */
    write_byte(reg.SP, get_r16(r16)&0xFF);
    set_r16(r16, (W<<8) + Z);
}


static void machine_push_r16_low(void) {
    /* push r16 low to stack*/
    write_byte(reg.SP, get_r16(r16)&0xFF);
}


static void machine_pop_Z_inc_sp(void) {
    /* pop into Z from stack, then inc sp */
    Z = read_byte(reg.SP);
    reg.SP++;
}


static void machine_pop_W_inc_sp(void) {
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


static void machine_dec_sp_inc_pc(void) {
    /* decrement sp without setting flags and increment pc*/
    reg.SP--;
    reg.PC++;
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
    working_bit = (int8_t)Z>0; // set if Z is positive
    working16bit += (int8_t)Z;
    set_flag(ZFLAG, 0);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, (working16bit&255)<Z);
    set_flag(HFLAG, (working16bit&15)<(Z&15));
    Z = working16bit&0xFF;
    W = working16bit>>8;
}


static void machine_add_W_sph_c(void) {
    /* add or subtract C flag from SP high and save to W */
    if (W) {
        if (working_bit) {
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
    working_bit = byte > get_r8(R8L);
}

static void machine_add_h_r16h(void) {
    /* add h and the carry from r16 low to r16 high and save to h */
    uint8_t byte = get_r16(r16)>>8;
    uint16_t word = get_r8(R8H) + byte + working_bit;
    byte = (get_r8(R8H)&15) + (byte&15) + working_bit;
    set_r8(R8H, (uint8_t)word);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, word>>8>0);
    set_flag(HFLAG, byte>>4>0);
    reg.PC++;
}


static void machine_add_Z(void) {
    /* add Z and carry to A, store result to A */
    uint16_t word = get_r8(R8A) + Z + working_bit;
    uint8_t byte = (get_r8(R8A)&15) + (Z&15) + working_bit;
    set_r8(R8A, (uint8_t)word);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 0);
    set_flag(CFLAG, word>>8>0);
    set_flag(HFLAG, byte>>4>0);
    reg.PC++;
}


static void machine_sub_Z(void) {
    /* subtract Z and carry from A, store result to A */
    uint16_t word = get_r8(R8A) - Z - working_bit;
    uint8_t byte = (get_r8(R8A)&15) - (Z&15) - working_bit;
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
    set_r8(R8A, get_r8(R8A) & Z);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 0);
    set_flag(HFLAG, 1);
    set_flag(CFLAG, 0);
    reg.PC++;
}


static void machine_or_Z(void) {
    /* logical or A and Z, store result to A */
    set_r8(R8A, get_r8(R8A) | Z);
    set_flag(ZFLAG, get_r8(R8A)==0);
    set_flag(NFLAG, 0);
    set_flag(HFLAG, 0);
    set_flag(CFLAG, 0);
    reg.PC++;
}


static void machine_xor_Z(void) {
    /* logical xor A and Z, store result to A */
    set_r8(R8A, get_r8(R8A) ^ Z);
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
    set_flag(ZFLAG, do_zflag&&(W==0));
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rlc_addr_Z(void) {
    /* rotate left circular Z, store in addr */
    set_flag(CFLAG, Z>>7);
    W = (Z << 1) + get_flag(CFLAG);
    write_byte(addr, W);
    set_flag(ZFLAG, W==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_rrc_r8(void) {
    /* rotate right circular r8 */
    set_flag(CFLAG, get_r8(r8)&1);
    W = (get_r8(r8) >> 1) + (get_flag(CFLAG)<<7);
    set_r8(r8, W);
    set_flag(ZFLAG, do_zflag&&(W==0));
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rrc_addr_Z(void) {
    /* rotate right circular Z, store in addr */
    set_flag(CFLAG, Z&1);
    W = (Z >> 1) + (get_flag(CFLAG)<<7);
    write_byte(addr, W);
    set_flag(ZFLAG, W==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_rl_r8(void) {
    /* rotate left r8*/
    working_bit = get_flag(CFLAG);
    set_flag(CFLAG, get_r8(r8)>>7);
    W = (get_r8(r8) << 1) + working_bit;
    set_r8(r8, W);
    set_flag(ZFLAG, do_zflag&&(W==0));
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rl_addr_Z(void) {
    /* rotate left Z, store in addr */
    working_bit = get_flag(CFLAG);
    set_flag(CFLAG, Z>>7);
    W = (Z << 1) + working_bit;
    write_byte(addr, W);
    set_flag(ZFLAG, W==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_rr_r8(void) {
    /* rotate right r8 */
    working_bit = get_flag(CFLAG);
    set_flag(CFLAG, get_r8(r8)&1);
    W = (get_r8(r8) >> 1) + (working_bit<<7);
    set_r8(r8, W);
    set_flag(ZFLAG, do_zflag&&(W==0));
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_rr_addr_Z(void) {
    /* rotate right Z, store in addr */
    working_bit = get_flag(CFLAG);
    set_flag(CFLAG, Z&1);
    W = (Z >> 1) + (working_bit<<7);
    write_byte(addr, W);
    set_flag(ZFLAG, W==0);
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


static void machine_sla_addr_Z(void) {
    /* shift left arithmetic Z, store in addr */
    set_flag(CFLAG, Z >> 7);
    Z = Z<<1;
    write_byte(addr, Z);
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


static void machine_sra_addr_Z(void) {
    /* shift right arithmetic Z, store in addr */
    set_flag(CFLAG, Z&1);
    Z = (Z>>1) + (Z&128);
    write_byte(addr, Z);
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


static void machine_swap_addr_Z(void) {
    /* swap nybbles in Z, store in addr */
    W = Z << 4;
    Z = W + (Z >> 4);
    write_byte(addr, Z);
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


static void machine_srl_addr_Z(void) {
    /* shift right logically Z, store in addr */
    set_flag(CFLAG, Z&1);
    Z = Z>>1;
    write_byte(addr, Z);
    set_flag(ZFLAG, Z==0);
    set_flag(HFLAG, 0);
    set_flag(NFLAG, 0);
}


static void machine_bit_W_r8(void) {
    /* test bit W in r8 */
    set_flag(ZFLAG, (get_r8(r8) & (1<<W))==0);
    set_flag(HFLAG, 1);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_bit_W_Z(void) {
    /* test bit W in Z */
    set_flag(ZFLAG, (Z & (1<<W))==0);
    set_flag(HFLAG, 1);
    set_flag(NFLAG, 0);
    reg.PC++;
}


static void machine_res_W_r8(void) {
    /* reset bit W in r8 */
    set_r8(r8, get_r8(r8) & ~(1<<W));
    reg.PC++;
}


static void machine_res_addr_W_Z(void) {
    /* reset bit W in r8 and store in addr */
    write_byte(addr, Z & ~(1<<W));
}


static void machine_set_W_r8(void) {
    /* set bit W in r8 */
    set_r8(r8, get_r8(r8) | (1<<W));
    reg.PC++;
}


static void machine_set_addr_W_Z(void) {
    /* set bit W in r8 and store in addr */
    write_byte(addr, Z | (1<<W));
}


static void machine_add_WZ_r16_Z_1(void) {
    /* set WZ to r16 + signed Z + 1 */
    uint16_t working16bit = get_r16(r16);
    working16bit += (int8_t)Z + 1;
    W = working16bit >> 8;
    Z = working16bit & 0xFF;
}


static void machine_ime_enable(void) {
    /* enable IME */
    set_ime(1);
}


static void machine_ime_enable_late(void) {
    /* enable IME */
    do_ei_set = 1;
    reg.PC++;
}


static void machine_ime_disable(void) {
    /* disable IME (happens on time) */
    set_ime(0);
    reg.PC++;
}


static void machine_stop(void) {
    /* handle entering STOP mode */
    if ((*(ram+0xFF00)&0x0F)<0xF) { //button is being held
        if (*(ram+0xFF0F)&*(ram+0xFFFF)) { // interrupt is pending
            reg.PC++; // stop is 1-byte, no HALT, no DIV reset
        } else {
            reg.PC += 2;
            do_haltmode = 1; // stop is 2-byte, HALT mode, no DIV reset
        }
    } else {
        if (*(ram+0xFF0F)&*(ram+0xFFFF)) {
            reg.PC++;
            do_haltmode = 2;
            *(ram+0xFF04) = 0; // stop is 1-byte, STOP mode, DIV reset
        } else {
            reg.PC += 2;
            do_haltmode = 2;
            *(ram+0xFF04) = 0; // stop is 2-byte, STOP mode, DIV reset
        }
    }
}


static void machine_halt(void) {
    /* handle entering HALT mode */
    do_haltmode = 1;
    reg.PC++;
}


static void machine_set_pc_addr(void) {
    /* set PC to addr, usd in interrupt handling */
    reg.PC = addr;
}


/*************************************************************************************************************************************************************/

static void instr_nop(void) {
    /* do nothing */
    scheduled_instructions[0] = &machine_nop;
    num_scheduled_instructions = 1;
}


static void instr_invalid(void) {
    /* crash */
    fprintf(stderr, "Error: Encountered an invalid opcode (0x%.2x)\n", read_byte(reg.PC));
    exit(EXIT_FAILURE);
}


static void instr_ld_r8_r8(void) {
    /* load into register r8 from register r */
    if (((read_byte(reg.PC)>>3)&7) == R8B && (read_byte(reg.PC)&7) == R8B) {
        //breakpoint
        if (reg.BC == 0x0305 && reg.DE == 0x080d && reg.HL == 0x1522) { //MTS Success
            printf("BREAKPOINT SUCCESS B/C/D/E/H/L = %.2x/%.2x/%.2x/%.2x/%.2x/%.2x\n", get_r8(R8B), get_r8(R8C), get_r8(R8D), get_r8(R8E), get_r8(R8H), get_r8(R8L));
            //exit(EXIT_SUCCESS);
        } else if (reg.BC == 0x4242 && reg.DE == 0x4242 && reg.HL == 0x4242) { //MTS Failure
            printf("BREAKPOINT FAILURE B/C/D/E/H/L = %.2x/%.2x/%.2x/%.2x/%.2x/%.2x\n", get_r8(R8B), get_r8(R8C), get_r8(R8D), get_r8(R8E), get_r8(R8H), get_r8(R8L));
            //exit(EXIT_FAILURE);
        } else {
            printf("BREAKPOINT UNKNOWN B/C/D/E/H/L = %.2x/%.2x/%.2x/%.2x/%.2x/%.2x\n", get_r8(R8B), get_r8(R8C), get_r8(R8D), get_r8(R8E), get_r8(R8H), get_r8(R8L));
        }
    }


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
    r8 = read_byte(reg.PC)&7;
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_r8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    num_scheduled_instructions = 2;
}


static void instr_ld_hl_imm8(void) {
    /* load into byte pointed to by hl */
    addr = reg.HL;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_addr_Z;
    scheduled_instructions[2] = &machine_idle;
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
    addr = 0xFF00 + get_r8(R8C);
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_load_r8_Z;
    num_scheduled_instructions = 2;
}


static void instr_ldh_c_a(void) {
    /* load into the byte in 0xFF00 + C from register A */
    r8 = R8A;
    addr = 0xFF00 + get_r8(R8C);
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
    scheduled_instructions[2] = &machine_load_addr_r16_low;
    scheduled_instructions[3] = &machine_inc_addr;
    scheduled_instructions[4] = &machine_load_addr_r16_high;
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
    r16 = decode_r16stk((read_byte(reg.PC)>>4)&3);
    scheduled_instructions[0] = &machine_dec_sp;
    scheduled_instructions[1] = &machine_push_r16_high_dec_sp;
    scheduled_instructions[2] = &machine_push_r16_low;
    scheduled_instructions[3] = &machine_nop;
    num_scheduled_instructions = 4;
}


static void instr_pop_r16(void) {
    /* pop r16 from stack */
    r16 = decode_r16stk((read_byte(reg.PC)>>4)&3);
    scheduled_instructions[0] = &machine_pop_Z_inc_sp;
    scheduled_instructions[1] = &machine_pop_W_inc_sp;
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
    Z = get_r8(read_byte(reg.PC)&7);
    working_bit = 0;
    scheduled_instructions[0] = &machine_add_Z;
    num_scheduled_instructions = 1;
}


static void instr_add_a_hl(void) {
    /* add A to byte pointed to by hl and store result in A */
    addr = reg.HL;
    working_bit = 0;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_add_a_imm8(void) {
    /* add A to imm8 and store result in A */
    working_bit = 0;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_adc_a_r8(void) {
    /* add A to r8 plus carry and and store result in A */
    Z = get_r8(read_byte(reg.PC)&7);;
    working_bit = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_add_Z;
    num_scheduled_instructions = 1;
}


static void instr_adc_a_hl(void) {
    /* add A to byte pointed to by hl plus carry and store result in A */
    addr = reg.HL;
    working_bit = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_adc_a_imm8(void) {
    /* add A to imm8 plus carry and store result in A */
    working_bit = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_add_Z;
    num_scheduled_instructions = 2;
}


static void instr_sub_a_r8(void) {
    /* subtract A and r8 and store result in A */
    Z = get_r8(read_byte(reg.PC)&7);;
    working_bit = 0;
    scheduled_instructions[0] = &machine_sub_Z;
    num_scheduled_instructions = 1;
}


static void instr_sub_a_hl(void) {
    /* subtract A and byte pointed to by hl and store result in A */
    addr = reg.HL;
    working_bit = 0;
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_sub_a_imm8(void) {
    /* subtract A and imm8 and store result in A */
    working_bit = 0;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_sbc_a_r8(void) {
    /* subtract A and r8 plus carry and and store result in A */
    Z = get_r8(read_byte(reg.PC)&7);;
    working_bit = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_sub_Z;
    num_scheduled_instructions = 1;
}


static void instr_sbc_a_hl(void) {
    /* subtract A and byte pointed to by hl plus carry and store result in A */
    addr = reg.HL;
    working_bit = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_addr;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_sbc_a_imm8(void) {
    /* subtract A and imm8 plus carry and store result in A */
    working_bit = get_flag(CFLAG);
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_sub_Z;
    num_scheduled_instructions = 2;
}


static void instr_cmp_a_r8(void) {
    /* subtract A and r8 but do not store result */
    Z = get_r8(read_byte(reg.PC)&7);;
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
    Z = get_r8(read_byte(reg.PC)&7);;
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
    Z = get_r8(read_byte(reg.PC)&7);;
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
    Z = get_r8(read_byte(reg.PC)&7);;
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


static void instr_scf(void) {
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
    do_zflag = 0;
    scheduled_instructions[0] = &machine_rlc_r8;
    num_scheduled_instructions = 1;
}


static void instr_rrca(void) {
    /* rotate right circular accumulator */
    r8 = R8A;
    do_zflag = 0;
    scheduled_instructions[0] = &machine_rrc_r8;
    num_scheduled_instructions = 1;
}


static void instr_rla(void) {
    /* rotate left accumulator */
    r8 = R8A;
    do_zflag = 0;
    scheduled_instructions[0] = &machine_rl_r8;
    num_scheduled_instructions = 1;
}


static void instr_rra(void) {
    /* rotate right accumulator */
    r8 = R8A;
    do_zflag = 0;
    scheduled_instructions[0] = &machine_rr_r8;
    num_scheduled_instructions = 1;
}


static void instr_rlc_r8(void) {
    /* rotate left circular r8 */
    do_zflag = 1;
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_rlc_r8;
    num_scheduled_instructions = 2;
}


static void instr_rlc_hl(void) {
    /* rotate left circular the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_rlc_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_rrc_r8(void) {
    /* rotate right circular r8 */
    do_zflag = 1;
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_rrc_r8;
    num_scheduled_instructions = 2;
}


static void instr_rrc_hl(void) {
    /* rotate right circular the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_rrc_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_rl_r8(void) {
    /* rotate left r8 */
    do_zflag = 1;
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_rl_r8;
    num_scheduled_instructions = 2;
}


static void instr_rl_hl(void) {
    /* rotate left the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_rl_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_rr_r8(void) {
    /* rotate right r8 */
    do_zflag = 1;
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_rr_r8;
    num_scheduled_instructions = 2;
}


static void instr_rr_hl(void) {
    /* rotate right the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_rr_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_sla_r8(void) {
    /* shift left arithmetic r8 */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_sla_r8;
    num_scheduled_instructions = 2;
}


static void instr_sla_hl(void) {
    /* shift left arithmetic the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_sla_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_sra_r8(void) {
    /* shift right arithmetic r8 */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_sra_r8;
    num_scheduled_instructions = 2;
}


static void instr_sra_hl(void) {
    /* shift right arithmetic the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_sra_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_swap_r8(void) {
    /* swap nybbles in r8 */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_swap_r8;
    num_scheduled_instructions = 2;
}


static void instr_swap_hl(void) {
    /* swap nybbles in the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_swap_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_srl_r8(void) {
    /* shift right logically r8 */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_srl_r8;
    num_scheduled_instructions = 2;
}


static void instr_srl_hl(void) {
    /* shift right logically the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_srl_addr_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_bit_r8(void) {
    /* test bit b in r8 */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_bit_W_r8;
    num_scheduled_instructions = 2;
}


static void instr_bit_hl(void) {
    /* test bit b in the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_bit_W_Z;
    num_scheduled_instructions = 3;
}


static void instr_res_r8(void) {
    /* reset bit b in r8 */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_res_W_r8;
    num_scheduled_instructions = 2;
}


static void instr_res_hl(void) {
    /* reset bit b in the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_res_addr_W_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}


static void instr_set_r8(void) {
    /* set bit b in r8 */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_set_W_r8;
    num_scheduled_instructions = 2;
}


static void instr_set_hl(void) {
    /* set bit b in the byte pointed to by hl */
    scheduled_instructions[0] = &machine_consume_prefix;
    scheduled_instructions[1] = &machine_load_Z_addr;
    scheduled_instructions[2] = &machine_set_addr_W_Z;
    scheduled_instructions[3] = &machine_nop; // free address bus to load next opcode
    num_scheduled_instructions = 4;
}

static void instr_jp_imm16(void) {
    /* unconditional jump to imm16 */
    r16 = R16PC;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_W_imm8;
    scheduled_instructions[2] = &machine_load_r16_WZ;
    scheduled_instructions[3] = &machine_idle; // wait a cycle, do not change PC
    num_scheduled_instructions = 4;
}


static void instr_jp_hl(void) {
    /* unconditional jump to hl */
    W = reg.HL >> 8;
    Z = reg.HL & 0xFF;
    r16 = R16PC;
    scheduled_instructions[0] = &machine_load_r16_WZ;
    num_scheduled_instructions = 1;
}


static void instr_jp_cc_imm16(void) {
    /* conditional jump to hl */
    r16 = R16PC;
    working_bit = is_cc((read_byte(reg.PC)>>3)&3);
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_W_imm8;
    if (working_bit) {
        scheduled_instructions[2] = &machine_load_r16_WZ;
        scheduled_instructions[3] = &machine_idle; // wait a cycle, do not change PC
        num_scheduled_instructions = 4;
    } else {
        scheduled_instructions[2] = &machine_nop;
        num_scheduled_instructions = 3;
    }

}


static void instr_jr_imm8(void) {
    /* relative jump to PC + signed imm8 */
    r16 = R16PC;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_add_WZ_r16_Z_1;
    scheduled_instructions[2] = &machine_load_r16_WZ;
    num_scheduled_instructions = 3;
    
}


static void instr_jr_cc_imm8(void) {
    /* conditional relative jump to PC + signed imm8 */
    r16 = R16PC;
    working_bit = is_cc((read_byte(reg.PC)>>3)&3);
    scheduled_instructions[0] = &machine_load_Z_imm8;
    if (working_bit) {
        scheduled_instructions[1] = &machine_add_WZ_r16_Z_1;
        scheduled_instructions[2] = &machine_load_r16_WZ;
        num_scheduled_instructions = 3;
    } else {
        scheduled_instructions[1] = &machine_nop;
        num_scheduled_instructions = 2;
    }
    
}


static void instr_call_imm16(void) {
    /* unconditional call to imm16 */
    r16 = R16PC;
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_W_imm8;
    scheduled_instructions[2] = &machine_dec_sp_inc_pc;
    scheduled_instructions[3] = &machine_push_r16_high_dec_sp;
    scheduled_instructions[4] = &machine_push_r16_low_set_r16_WZ;
    scheduled_instructions[5] = &machine_idle;
    num_scheduled_instructions = 6;
}


static void instr_call_cc_imm16(void) {
    /* conditional call to imm16 */
    r16 = R16PC;
    working_bit = is_cc((read_byte(reg.PC)>>3)&3);
    scheduled_instructions[0] = &machine_load_Z_imm8;
    scheduled_instructions[1] = &machine_load_W_imm8;
    if (working_bit) {
        scheduled_instructions[2] = &machine_dec_sp_inc_pc;
        scheduled_instructions[3] = &machine_push_r16_high_dec_sp;
        scheduled_instructions[4] = &machine_push_r16_low_set_r16_WZ;
        scheduled_instructions[5] = &machine_idle;
        num_scheduled_instructions = 6;
    } else {
        scheduled_instructions[2] = &machine_nop;
        num_scheduled_instructions = 3;
    }
}


static void instr_ret(void) {
    /* unconditional return */
    r16 = R16PC;
    scheduled_instructions[0] = &machine_pop_Z_inc_sp;
    scheduled_instructions[1] = &machine_pop_W_inc_sp;
    scheduled_instructions[2] = &machine_load_r16_WZ;
    scheduled_instructions[3] = &machine_idle;
    num_scheduled_instructions = 4;
}


static void instr_ret_cc(void) {
    /* conditional return */
    r16 = R16PC;
    working_bit = is_cc((read_byte(reg.PC)>>3)&3);
    scheduled_instructions[0] = &machine_idle;
    if (working_bit) {
        scheduled_instructions[0] = &machine_pop_Z_inc_sp;
        scheduled_instructions[1] = &machine_pop_W_inc_sp;
        scheduled_instructions[2] = &machine_load_r16_WZ;
        scheduled_instructions[3] = &machine_idle;
        num_scheduled_instructions = 4;
    } else {
        scheduled_instructions[1] = &machine_nop;
        num_scheduled_instructions = 2;
    }
}


static void instr_reti(void) {
    /* unconditional return and enable IME*/
    r16 = R16PC;
    scheduled_instructions[0] = &machine_pop_Z_inc_sp;
    scheduled_instructions[1] = &machine_pop_W_inc_sp;
    scheduled_instructions[2] = &machine_load_r16_WZ;
    scheduled_instructions[3] = &machine_ime_enable;
    num_scheduled_instructions = 4;
}


static void instr_rst(void) {
    /* call to vectorised address */
    r16 = R16PC;
    W = 0x00;
    Z = read_byte(reg.PC)&0x38;
    scheduled_instructions[0] = &machine_dec_sp_inc_pc;
    scheduled_instructions[1] = &machine_push_r16_high_dec_sp;
    scheduled_instructions[2] = &machine_push_r16_low_set_r16_WZ;
    scheduled_instructions[3] = &machine_idle;
    num_scheduled_instructions = 4;
}


static void instr_ei(void) {
    /* enable interrupts */  
    scheduled_instructions[0] = &machine_ime_enable_late;
    num_scheduled_instructions = 1;
}


static void instr_di(void) {
    /* disable interrupts */  
    scheduled_instructions[0] = &machine_ime_disable;
    num_scheduled_instructions = 1;
}


static void instr_stop(void) {
    /* enter STOP mode */
    scheduled_instructions[0] = &machine_stop;
    num_scheduled_instructions = 1;
}


static void instr_halt(void) {
    /* enter HALT mode */
    scheduled_instructions[0] = &machine_halt;
    num_scheduled_instructions = 1;
}


void queue_instruction(void) {
    /* read the opcode at the PC and queue an instruction's atomic instructions */
    uint8_t opcode = read_byte(get_r16(R16PC));

    if (opcode == 0xCB) {
        (prefixed_instructions[read_byte(get_r16(R16PC)+1)])();
    } else {
        (instructions[opcode])();
    }
    current_instruction_count = 0;
}


void load_interrupt_instructions(uint8_t isr) {
    /* load the atomic instructions that execute during interrupt handling */
    r16 = R16PC;
    addr = 0x40+(isr<<3);
    scheduled_instructions[0] = &machine_idle;
    scheduled_instructions[1] = &machine_dec_sp;
    scheduled_instructions[2] = &machine_push_r16_high_dec_sp;
    scheduled_instructions[3] = &machine_push_r16_low;
    scheduled_instructions[4] = &machine_set_pc_addr;
    num_scheduled_instructions = 5;
    current_instruction_count = 0;
    // set_ime(0); //disable isr flag
    // reg.SP-=2; //execute a CALL (push PC to stack)
    // write_word(reg.SP, reg.PC);
    // set_r16(R16PC, 0x40+(isr<<3)); // go to corresponding isr add
}