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

void handle_audio_register(uint16_t addr); // explicity define function declared in dependent translation unit

JoypadState joypad_state = {0,0,0,0,0,0,0,0}; //extern
Registers reg; //extern
bool LOOP = 1; //extern
uint8_t OAM_DMA_starter = 0; //extern
bool OAM_DMA = 0; //extern
uint16_t OAM_DMA_timeout = 0; //extern
uint16_t system_counter = 0xABCE; //extern
uint8_t TIMA_overflow_delay = 0; //extern
bool TIMA_overflow_flag = 0; //extern
bool timer_last_state = 0;
bool do_div_reset;
uint16_t div_reset_old_sysclk;
extern void (*write_MBANK_register)(uint16_t, uint8_t);
extern uint8_t (*read_rom)(uint32_t);
extern void (*write_ext_ram)(uint16_t, uint8_t);
extern uint8_t (*read_ext_ram)(uint16_t);

extern uint8_t* ram;
extern gbRom rom;

void increment_timers(void) {
    /* Handle the incrementing and overflowing of timers */
    if (do_div_reset) {
        do_div_reset = 0;
    } else {
        system_counter++;
    }
    bool timer_current_state = 0;
    if ((*(ram+REG_TAC)>>2)&1) { // is timer enabled in TAC
        switch ((*(ram+REG_TAC))&3) // Select timer speed
        {
        case 0: //4096 Hz
            timer_current_state = (system_counter>>9)&1;
            break;
        case 3: //16384 Hz
            timer_current_state = (system_counter>>7)&1;
            break;
        case 2: //65536 Hz
            timer_current_state = (system_counter>>5)&1;
            break;
        case 1: //262144 Hz
            timer_current_state = (system_counter>>3)&1;
            break;
        }
    }
    if (timer_last_state && !timer_current_state) { // falling edge
        (*(ram+REG_TIMA))++; // inc TIMA
        if (!*(ram+REG_TIMA)) TIMA_overflow_delay = 2; // trigger overflow
    }
    timer_last_state = timer_current_state;
}

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
    return ((((uint8_t)1<<flagname) & (uint8_t)reg.AF));
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
    return 0xFF;
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
    case R16AF:
        reg.AF = value & 0xFFF0;
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
    return 0xFF;
}


void set_isr_enable(uint8_t isr_type, bool state) {
    /* Enable or disable a particular type of interrupt */
    *(ram+REG_IE) &= ~(1<<isr_type); //clear bit
    *(ram+REG_IE) |= (state<<isr_type); //set bit
}


void write_byte(uint16_t addr, uint8_t byte) {
    /* Write a byte to a particular address. Ignores writing to protected RAM */
    if (OAM_DMA  && (addr >= 0xFE00 && addr < 0xFEA0)) return; // OAM is inaccessible during DMA

    if (addr == REG_DMA) { // enter DMA mode
        *(ram+addr) = byte;
        //printf("DMA: Scheduled start at sysclk=%.4x\n", system_counter);
        OAM_DMA_starter = 2;
        return;
    }

    if (addr < 0x8000) { //mbc registers
        write_MBANK_register(addr, byte);
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


    if (addr == REG_JOYP) { //writing to this addr queries the joypad
        *(ram+REG_JOYP) = *(ram+REG_JOYP)&0xCF;
        *(ram+REG_JOYP) |= byte&0x30; // set mask of byte
        joypad_io();
        return;
    }
    if (addr == REG_DIV) { //writing to DIV sets it to 0, but requires special timer behaviour
        div_reset_old_sysclk=system_counter;
        system_counter = 0;
        do_div_reset=1;
        increment_timers();
        return;
    }

    if (addr == REG_TIMA) { // writing to TIMA
        if (TIMA_overflow_delay == 2) {
            TIMA_overflow_delay = 0; // don't trigger overflow
            *(ram+addr) = byte;
            return;
        } 
        if (TIMA_overflow_flag) {
            *(ram+REG_TIMA) = *(ram+REG_TMA);
            return;
        }
    }
    if (addr == REG_TMA) { //writing to TMA
        if (TIMA_overflow_flag) *(ram+REG_TIMA) = byte;
        *(ram+addr) = byte;
        return;
    }


    if (addr == REG_LY) return;
    if (addr == REG_STAT) {
        *(ram+addr) &= 0x87;
        *(ram+addr) += byte & 0x78; // only set certain regs
        return;
    }

    // if ((*(ram+REG_STAT)&2) && (addr >= 0xFE00 && addr < 0xFEA0)) return; // OAM inaccessible
    // if ((*(ram+REG_STAT)&3) && (addr >= 0x8000 && addr < 0xA000)) return; // VRAM inaccessible

    if (addr >= 0xFF00 && addr < 0xFF80) { //Special instructions
        uint8_t mask = write_masks[addr&0xFF];
        *(ram+addr) &= ~mask; // set to-be-written bits low
        *(ram+addr) |= byte&mask; // write only the masked bits

        if (addr >= 0xFF10 && addr < 0xFF3F) { // Audio registers
            handle_audio_register(addr);
        }
        return;
    }

    *(ram+addr) = byte; // write if nothing else happens
}


uint8_t read_byte(uint16_t addr) {
    /* Read a byte from a particular address. Returns 0xFF on a read-protected register */
    if (OAM_DMA  && (addr >= 0xFE00 && addr < 0xFEA0)) return 0xFF; // OAM is inaccessible during DMA
    if (addr < 0x8000) return read_rom(addr); // Read from ROM

    if (addr >= 0xA000 && addr < 0xC000) { // Reading from external RAM
        return read_ext_ram(addr-0xA000);
    }

    if ((*(ram+REG_STAT)&2) && (addr >= 0xFE00 && addr < 0xFEA0)) return 0xFF; // OAM inaccessible
    if (((*(ram+REG_STAT)&3)==3) && (addr >= 0x8000 && addr < 0xA000)) return 0xFF; // VRAM inaccessible

    if (addr == REG_DIV) { //reading DIV
        return system_counter>>8;
    }

    if (addr == REG_JOYP) { //reading joypad
        joypad_io();
        return *(ram+addr);
    }

    if (addr >= 0xFF00 && addr < 0xFF80) { //Special instructions
        return *(ram+addr) | (read_masks[addr&0xFF]); // set unreadable bits high
    }

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


void read_dma(void) {
    /* read a byte from the appropriate location for DMA */
    if  (*(ram+REG_DMA) < 0x80) { // Read from ROM
        *(ram + 0xFE00 + OAM_DMA_timeout) = read_rom((*(ram+REG_DMA)<<8) + OAM_DMA_timeout);
    } else if (*(ram+REG_DMA) < 0xA0) { // Read from VRAM
        *(ram + 0xFE00 + OAM_DMA_timeout) = *(ram + (*(ram+REG_DMA)<<8) + OAM_DMA_timeout);
    } else if (*(ram+REG_DMA) < 0xC0) { // Read from External RAM
        *(ram + 0xFE00 + OAM_DMA_timeout) = read_ext_ram((*(ram+REG_DMA)<<8) + OAM_DMA_timeout);
    } else { // Read from WRAM
        uint8_t high_addr = *(ram+REG_DMA);
        if (high_addr >= 0xE0) high_addr -= 0x20;
        *(ram + 0xFE00 + OAM_DMA_timeout) = *(ram + (high_addr<<8) + OAM_DMA_timeout);
    }
    if (OAM_DMA_timeout==160) OAM_DMA = 0;
    OAM_DMA_timeout++;
}


void joypad_io(void) {

    uint8_t old_state = *(ram+REG_JOYP) & 0x0F;
    *(ram+REG_JOYP) |=0x0F; // set lower nibble high (no buttons pushed)
    if (!(*(ram+REG_JOYP)&32)) { // read SsBA
        *(ram+REG_JOYP) &= ~((joypad_state.select<<3) + (joypad_state.start<<2) + (joypad_state.B<<1) + (joypad_state.A));
    } 
    if (!(*(ram+REG_JOYP)&16)) { // read dpad
        *(ram+REG_JOYP) &= ~((joypad_state.down<<3) + (joypad_state.up<<2) + (joypad_state.left<<1) + (joypad_state.right));
    }
    if (old_state & ~(*(ram+REG_JOYP) & 0x0F)) {// if any bits were high and are now low
        *(ram+REG_IF) |= 16; // Request a joypad interrupt
    }
}