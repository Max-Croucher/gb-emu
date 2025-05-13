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

Registers init_registers(void) {
    /* Initialise the gameboy registers, with appropriate PC */
    Registers reg = {0, 0, 0, 0, 0, PROG_START};
    return reg;
}

void print_registers(Registers *reg) {
    /* Print a textual representation of the current CPU registers */
    printf("\nCPU State:   ");
    printf("ZNHC: %d%d%d%d\n", get_flag(reg, ZFLAG), get_flag(reg, NFLAG), get_flag(reg, HFLAG), get_flag(reg, CFLAG));
    printf("A : 0x%.2x     ", (*reg).AF>>8);
    printf("BC: 0x%.4x\n", (*reg).BC);
    printf("DE: 0x%.4x   ", (*reg).DE);
    printf("HL: 0x%.4x\n", (*reg).HL);
    printf("SP: 0x%.4x   ", (*reg).SP);
    printf("PC: 0x%.4x\n", (*reg).PC);
}


uint8_t get_flag(Registers *reg, uint8_t flagname) {
    /* get the state associated with the given flag */
    return ((((uint8_t)1<<flagname) & (uint8_t)(*reg).AF))&&1;
}


void set_flag(Registers *reg, uint8_t flagname, bool state) {
    /* set the state of the given flag */
    (*reg).AF &= ~((uint8_t)1<<flagname); // clear bit
    (*reg).AF |= (state<<flagname); // set bit
}


uint8_t get_r8(Registers *reg, uint8_t regname) {
    /* get the state associated with the given 8-bit reg */
    switch (regname)
    {
    case R8A:
        return (*reg).AF >> 8;
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
    
    default:
        return 0;
    }
}


void set_r8(Registers *reg, uint8_t regname, uint8_t value) {
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