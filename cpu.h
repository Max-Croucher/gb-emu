/* Header file for cpu.c, controlling the execution of the gameboy cpu
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef CPU_H
#define CPU_H

typedef struct {
    uint16_t AF;
    uint16_t BC;
    uint16_t DE;
    uint16_t HL;
    uint16_t SP;
    uint16_t PC;
} Registers;

#define PROG_START 0x0100

#define ZFLAG 7 //define flags
#define NFLAG 6
#define HFLAG 5
#define CFLAG 4

#define R8B 0 //define 8-bit registers
#define R8C 1
#define R8D 2
#define R8E 3
#define R8H 4
#define R8L 5
#define R8A 7 

#define R16BC 0 //define 16-bit registers
#define R16DE 1
#define R16HL 2
#define R16SP 3
#define R16PC 4

Registers init_registers(void);
void print_registers(Registers *reg);
uint8_t get_flag(Registers *reg, uint8_t flagname);
void set_flag(Registers *reg, uint8_t flagname, bool state);
uint8_t get_r8(Registers *reg, uint8_t regname);
void set_r8(Registers *reg, uint8_t regname, uint8_t value);
uint16_t get_r16(Registers *reg, uint16_t regname);
void set_r16(Registers *reg, uint16_t regname, uint16_t value);
bool is_cc(Registers *reg, uint8_t cond);

#endif // CPU_H