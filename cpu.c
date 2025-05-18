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

static int joypad_io_state = 0;
static JoypadState joypad_state = {0,0,0,0,0,0,0,0};
bool TIMA_oddity = 0;
uint8_t vblank_mode = 0;

Registers init_registers(void) {
    /* Initialise the gameboy registers, with appropriate PC */
    Registers reg = {0x01B0, 0x0013, 0x00D8, 0x014D, 0xFFFE, PROG_START, 0};
    return reg;
}

void print_registers(Registers *reg) {
    /* Print a textual representation of the current CPU registers */
    printf("\nCPU State:  ");
    printf("ZNHCI: %d%d%d%d%d\n", get_flag(reg, ZFLAG), get_flag(reg, NFLAG), get_flag(reg, HFLAG), get_flag(reg, CFLAG), (*reg).IME);
    printf("A : 0x%.2x      ", (*reg).AF>>8);
    printf("BC: 0x%.4x\n", (*reg).BC);
    printf("DE: 0x%.4x    ", (*reg).DE);
    printf("HL: 0x%.4x\n", (*reg).HL);
    printf("SP: 0x%.4x    ", (*reg).SP);
    printf("PC: 0x%.4x\n", (*reg).PC);
}


bool get_flag(Registers *reg, uint8_t flagname) {
    /* get the state associated with the given flag */
    return ((((uint8_t)1<<flagname) & (uint8_t)(*reg).AF))&&1;
}


void set_flag(Registers *reg, uint8_t flagname, bool state) {
    /* set the state of the given flag */
    (*reg).AF &= ~((uint8_t)1<<flagname); // clear bit
    (*reg).AF |= (state<<flagname); // set bit
}


uint8_t get_r8(Registers *reg, uint8_t* ram, uint8_t regname) {
    /* get the state associated with the given 8-bit reg */
    switch (regname)
    {
    case R8A:
        return (*reg).AF >> 8;
    case R8F:
        return (*reg).AF & 0xFF;
    case R8B:
        return (*reg).BC >> 8;
    case R8C:
        return (*reg).BC & 0xFF;
    case R8D:
        return (*reg).DE >> 8;
    case R8E:
        return (*reg).DE & 0xFF;
    case R8H:
        return (*reg).HL >> 8;
    case R8L:
        return (*reg).HL & 0xFF;
    case 6:
        return read_byte(ram, (*reg).HL);
    default:;
        char msg[32];
        sprintf(msg, "Invalid register id (%d).", regname);
        print_error(msg);
    }
}


void set_r8(Registers *reg, uint8_t* ram, uint8_t regname, uint8_t value) {
    /* set the state of the given 8-bit reg */
        switch (regname)
    {
    case R8A:
        (*reg).AF &= 0x00FF; //clear
        (*reg).AF |= (uint16_t)value << 8; //set
        break;
    case R8B:
        (*reg).BC &= 0x00FF; //clear
        (*reg).BC |= (uint16_t)value << 8; //set
        break;
    case R8C:
        (*reg).BC &= 0xFF00; //clear
        (*reg).BC |= value; //set
        break;
    case R8D:
        (*reg).DE &= 0x00FF; //clear
        (*reg).DE |= (uint16_t)value << 8; //set
        break;
    case R8E:
        (*reg).DE &= 0xFF00; //clear
        (*reg).DE |= value; //set
        break;
    case R8H:
        (*reg).HL &= 0x00FF; //clear
        (*reg).HL |= (uint16_t)value << 8; //set
        break;
    case R8L:
        (*reg).HL &= 0xFF00; //clear
        (*reg).HL |= value; //set
        break;
    case 6:
        write_byte(ram, (*reg).HL, value);
        break;
    default:;
        char msg[32];
        sprintf(msg, "Invalid register id (%d).", regname);
        print_error(msg);
    }
}


uint16_t get_r16(Registers *reg, uint16_t regname) {
    /* get the state associated with the given 16-bit reg */
    switch (regname)
    {
    case R16BC:
        return (*reg).BC;
    case R16DE:
        return (*reg).DE;
    case R16HL:
        return (*reg).HL;
    case R16SP:
        return (*reg).SP;
    case R16PC:
        return (*reg).PC;
    case R16AF:
        return (*reg).AF;
    default:
        return 0;
    }
}


void set_r16(Registers *reg, uint16_t regname, uint16_t value) {
    /* set the state of the given 16-bit reg */
        switch (regname)
    {
    case R16BC:
        (*reg).BC = value;
        break;
    case R16DE:
        (*reg).DE = value;
        break;
    case R16HL:
        (*reg).HL = value;
        break;
    case R16SP:
        (*reg).SP = value;
        break;
    case R16PC:
        (*reg).PC = value;
        break;
    }
}


bool is_cc(Registers *reg, uint8_t cond) {
    /* checks if a condition is met */
    switch (cond)
    {
    case 0b00: // is Z not set
        return !get_flag(reg, ZFLAG);
    case 0b01: // is Z not set
        return get_flag(reg, ZFLAG);
    case 0b10: // is C not set
        return !get_flag(reg, CFLAG);
    case 0b11: // is C set
        return get_flag(reg, CFLAG);
    
    default:
        fprintf(stderr, "Error: Unknown condition code!");
        exit(EXIT_FAILURE);
    }
}


void set_ime(Registers *reg, bool state) {
    /* set the Interrupt Master Enable */
    (*reg).IME = state;
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


void set_isr_enable(uint8_t *ram, uint8_t isr_type, bool state) {
    /* Enable or disable a particular type of interrupt */
    *(ram+0xFFFF) &= ~(1<<isr_type); //clear bit
    *(ram+0xFFFF) |= (state<<isr_type); //set bit
}


void write_byte(uint8_t *ram, uint16_t addr, uint8_t byte) {
    /* Write a byte to a particular address. Ignores writing to protected RAM */
    if (addr < 0x8000) { //mbc registers
        if (addr < 0x2000) {
            mbank_register(ram, 0, byte);
        } else if (addr < 0x4000) {
            mbank_register(ram, 1, byte);
        } else if (addr < 0x6000) {
            mbank_register(ram, 2, byte);
        }  else {
            mbank_register(ram, 3, byte);
        }
        return;
    }
    if (addr >= 0xE000 && addr < 0xFE00) {
        write_byte(ram, addr-0x2000, byte); //Echo RAM
        return;
    }
    if (addr >= 0xFEA0 && addr < 0xFEFF) return;

    if (addr == 0xFF00) joypad_io(ram); //writing to this addr queries the joypad

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


    if ((vblank_mode == 2 || vblank_mode == 3) && (addr >= 0xFE00 && addr < 0xFEA0)) return; // OAM inaccessible
    if ((vblank_mode == 3) && (addr >= 0x8000 && addr < 0xA000)) return; // VRAM inaccessible


    *(ram+addr) = byte; // write if no return

}


uint8_t read_byte(uint8_t *ram, uint16_t addr) {
    /* Read a byte from a particular address. Returns 0xFF on a read-protected register */
    if ((vblank_mode == 2 || vblank_mode == 3) && (addr >= 0xFE00 && addr < 0xFEA0)) return 0xFF; // OAM inaccessible
    if ((vblank_mode == 3) && (addr >= 0x8000 && addr < 0xA000)) return 0xFF; // VRAM inaccessible
    return *(ram+addr);
}


void write_word(uint8_t *ram, uint16_t addr, uint16_t word) {
    /* Write two bytes to a particular address and the next. Ignores writing to protected RAM */
    write_byte(ram, addr+1, word >> 8);
    write_byte(ram, addr, word&0xFF);
}


uint16_t read_word(uint8_t *ram, uint16_t addr) {
    /* Read a word from a particular address and the next. */
    return (((uint16_t)read_byte(ram, addr+1)) << 8) + read_byte(ram, addr);
}


void joypad_io(uint8_t* ram) {
    switch (joypad_io_state)
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

void set_render_blocking_mode(uint8_t mode) {
    /* Set the current rendering mode, to indicate which vram blocks are inaccessible */
    vblank_mode = mode;
}