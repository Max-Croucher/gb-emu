/* Source file for rom.c, controlling the execution of the gameboy cpu
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
#include "registers.h"

JoypadState joypad_state = {0,0,0,0,0,0,0,0}; //extern
Registers reg; //extern
bool LOOP = 1; //extern
extern bool TIMA_oddity;
extern uint8_t* ram;
extern gbRom rom;

void init_registers(void) {
    /* Initialise the gameboy registers, with appropriate PC */
    reg = (Registers){0x01B0, 0x0013, 0x00D8, 0x014D, 0xFFFE, PROG_START, 0};
}

void print_registers(void) {
    /* Print a textual representation of the current CPU registers */
    printf("\nCPU State:  ");
    printf("ZNHCI: %d%d%d%d%d\n", get_flag(ZFLAG), get_flag(NFLAG), get_flag(HFLAG), get_flag(CFLAG), reg.IME);
    printf("A : 0x%.2x      ", reg.AF>>8);
    printf("BC: 0x%.4x\n", reg.BC);
    printf("DE: 0x%.4x    ", reg.DE);
    printf("HL: 0x%.4x\n", reg.HL);
    printf("SP: 0x%.4x    ", reg.SP);
    printf("PC: 0x%.4x\n", reg.PC);
}


bool get_flag(uint8_t flagname) {
    /* get the state associated with the given flag */
    return ((((uint8_t)1<<flagname) & (uint8_t)reg.AF))&&1;
}


void set_flag(uint8_t flagname, bool state) {
    /* set the state of the given flag */
    reg.AF &= ~((uint8_t)1<<flagname); // clear bit
    reg.AF |= (state<<flagname); // set bit
}


uint8_t get_r8(uint8_t regname) {
    /* get the state associated with the given 8-bit reg */
    switch (regname)
    {
    case R8A:
        return reg.AF >> 8;
    case R8F:
        return reg.AF & 0xFF;
    case R8B:
        return reg.BC >> 8;
    case R8C:
        return reg.BC & 0xFF;
    case R8D:
        return reg.DE >> 8;
    case R8E:
        return reg.DE & 0xFF;
    case R8H:
        return reg.HL >> 8;
    case R8L:
        return reg.HL & 0xFF;
    case 6:
        return read_byte(reg.HL);
    default:;
        char msg[32];
        sprintf(msg, "Invalid register id (%d).", regname);
        print_error(msg);
    }
}


void set_r8(uint8_t regname, uint8_t value) {
    /* set the state of the given 8-bit reg */
        switch (regname)
    {
    case R8A:
        reg.AF &= 0x00FF; //clear
        reg.AF |= (uint16_t)value << 8; //set
        break;
    case R8B:
        reg.BC &= 0x00FF; //clear
        reg.BC |= (uint16_t)value << 8; //set
        break;
    case R8C:
        reg.BC &= 0xFF00; //clear
        reg.BC |= value; //set
        break;
    case R8D:
        reg.DE &= 0x00FF; //clear
        reg.DE |= (uint16_t)value << 8; //set
        break;
    case R8E:
        reg.DE &= 0xFF00; //clear
        reg.DE |= value; //set
        break;
    case R8H:
        reg.HL &= 0x00FF; //clear
        reg.HL |= (uint16_t)value << 8; //set
        break;
    case R8L:
        reg.HL &= 0xFF00; //clear
        reg.HL |= value; //set
        break;
    case 6:
        write_byte(reg.HL, value);
        break;
    default:;
        char msg[32];
        sprintf(msg, "Invalid register id (%d).", regname);
        print_error(msg);
    }
}


uint16_t get_r16(uint16_t regname) {
    /* get the state associated with the given 16-bit reg */
    switch (regname)
    {
    case R16BC:
        return reg.BC;
    case R16DE:
        return reg.DE;
    case R16HL:
        return reg.HL;
    case R16SP:
        return reg.SP;
    case R16PC:
        return reg.PC;
    case R16AF:
        return reg.AF;
    default:
        return 0;
    }
}


void set_r16(uint16_t regname, uint16_t value) {
    /* set the state of the given 16-bit reg */
        switch (regname)
    {
    case R16BC:
        reg.BC = value;
        break;
    case R16DE:
        reg.DE = value;
        break;
    case R16HL:
        reg.HL = value;
        break;
    case R16SP:
        reg.SP = value;
        break;
    case R16PC:
        reg.PC = value;
        break;
    }
}


bool is_cc(uint8_t cond) {
    /* checks if a condition is met */
    switch (cond)
    {
    case 0b00: // is Z not set
        return !get_flag(ZFLAG);
    case 0b01: // is Z not set
        return get_flag(ZFLAG);
    case 0b10: // is C not set
        return !get_flag(CFLAG);
    case 0b11: // is C set
        return get_flag(CFLAG);
    
    default:
        fprintf(stderr, "Error: Unknown condition code!");
        exit(EXIT_FAILURE);
    }
}


void set_ime(bool state) {
    /* set the Interrupt Master Enable */
    reg.IME = state;
}


uint8_t decode_r16stk(uint8_t code) {
    /* decode the r16stk code into a usable register */
    switch (code)
    {
    case 0:
        return R16BC;
    case 1:
        return R16DE;
    case 2:
        return R16HL;
    case 3:
        return R16AF;
    
    default:
        fprintf(stderr, "Error: Unknown r16stk code!");
    }
}


void set_isr_enable(uint8_t isr_type, bool state) {
    /* Enable or disable a particular type of interrupt */
    *(ram+0xFFFF) &= ~(1<<isr_type); //clear bit
    *(ram+0xFFFF) |= (state<<isr_type); //set bit
}


void write_byte(uint16_t addr, uint8_t byte) {
    /* Write a byte to a particular address. Ignores writing to protected RAM */
    if (addr < 0x8000) { //mbc registers
        mbank_register(addr, byte);
        return;
    }

    if (addr >= 0xA000 && addr < 0xC000) { //Writing to external RAM
        write_ext_ram(addr-0xA000, byte);
        return;
    }

    if (addr >= 0xE000 && addr < 0xFE00) {
        write_byte(addr-0x2000, byte); //Echo RAM
        return;
    }
    if (addr >= 0xFEA0 && addr < 0xFEFF) return;




    if (addr == 0xFF00) { //writing to this addr queries the joypad
        *(ram+0xFF00) = *(ram+0xFF00)&0xCF;
        *(ram+0xFF00) |= byte&0x30; // set mask of byte
        joypad_io();
        return;
    }
    if (addr == 0xFF04) *(ram+addr) = 0; //writing to DIV sets it to 0
    //strange TIMA behaviour
    if (addr == 0xFF05) { //TIMA 
        if (!*(ram+addr)) {
            TIMA_oddity = 1;
            *(ram+addr) = byte;
            return;
        }
        if (*(ram+0xFF05) == *(ram+0xFF06)) return;
    }
    if (addr == 0xFF06 && (*(ram+0xFF05) == *(ram+0xFF06))) {//writing to TMA on overflow: write to both
        *(ram+0xFF05) = byte;
        *(ram+0xFF06) = byte;
        return;
    }
    if (addr == 0xFF44) return;
    if (addr == 0xFF41) {
        *(ram+addr) &= 0x87;
        *(ram+addr) += byte & 0x78; // only set certain regs
        return;
    }

    // if ((*(ram+0xFF41)&2) && (addr >= 0xFE00 && addr < 0xFEA0)) return; // OAM inaccessible
    // if ((*(ram+0xFF41)&3) && (addr >= 0x8000 && addr < 0xA000)) return; // VRAM inaccessible

    // if (addr > 0xFF00) { //Special instructions
    //     uint8_t mask = write_masks[addr&0xFF];
    //     *(ram+addr) &= ~mask; // set to-be-written bits low
    //     *(ram+addr) |= byte&mask; // write only the masked bits
    // }

    *(ram+addr) = byte; // write if nothing else happens
}


uint8_t read_byte(uint16_t addr) {
    /* Read a byte from a particular address. Returns 0xFF on a read-protected register */

    if (addr < 0x8000) return read_rom(addr); // Read from ROM

    if (addr >= 0xA000 && addr < 0xC000) { // Reading from external RAM
        return read_ext_ram(addr-0xA000);
    }

    if ((*(ram+0xFF41)&2) && (addr >= 0xFE00 && addr < 0xFEA0)) return 0xFF; // OAM inaccessible
    if (((*(ram+0xFF41)&3)==3) && (addr >= 0x8000 && addr < 0xA000)) return 0xFF; // VRAM inaccessible

    // if (addr > 0xFF00) { //Special instructions
    //     return *(ram+addr) | (write_masks[addr&0xFF]); // set unreadable bits high
    // }


    return *(ram+addr);
}


void write_word(uint16_t addr, uint16_t word) {
    /* Write two bytes to a particular address and the next. Ignores writing to protected RAM */
    write_byte(addr+1, word >> 8);
    write_byte(addr, word&0xFF);
}


uint16_t read_word(uint16_t addr) {
    /* Read a word from a particular address and the next. */
    return (((uint16_t)read_byte(addr+1)) << 8) + read_byte(addr);
}


void joypad_io(void) {
    switch ((*(ram+0xFF00)>>4)&3)
    {
    case 0:
    case 3:
        *(ram+0xFF00) |=0x0F;
        break;
    case 1:
        *(ram+0xFF00) &=0xF0;
        *(ram+0xFF00) |= (!joypad_state.down<<3) + (!joypad_state.up<<2) + (!joypad_state.left<<1) + (!joypad_state.right);
        break;
    case 2:
        *(ram+0xFF00) &=0xF0;
        *(ram+0xFF00) |= (!joypad_state.select<<3) + (!joypad_state.start<<2) + (!joypad_state.B<<1) + (!joypad_state.A);
        break;
    }
}