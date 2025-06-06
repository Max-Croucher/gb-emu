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
    bool IME;
} Registers;


typedef struct {
  bool select;
  bool start;
  bool B;
  bool A;
  bool down;
  bool up;
  bool left;
  bool right;
} JoypadState;

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
#define R8F 8 // shouldn't usually be used

#define R16BC 0 //define 16-bit registers
#define R16DE 1
#define R16HL 2
#define R16SP 3
#define R16PC 4
#define R16AF 5

#define ISR_VBLANK 0 //define ISR type offsets
#define ISR_LCD 1
#define ISR_TIMER 2
#define ISR_SERIAL 3
#define ISR_JOYPAD 4

void increment_timers(void);
void init_registers(void);
void print_registers();
bool get_flag(uint8_t flagname);
void set_flag(uint8_t flagname, bool state);
uint8_t get_r8(uint8_t regname);
void set_r8(uint8_t regname, uint8_t value);
uint16_t get_r16(uint16_t regname);
void set_r16(uint16_t regname, uint16_t value);
bool is_cc(uint8_t cond);
void set_ime(bool state);
uint8_t decode_r16stk(uint8_t);
void set_isr_enable(uint8_t isr_type, bool state);
void write_byte(uint16_t addr, uint8_t byte);
uint8_t read_byte(uint16_t addr);
void write_word(uint16_t addr, uint16_t word);
uint16_t read_word(uint16_t addr);
uint8_t read_dma(void);
void joypad_io(void);

#endif // CPU_H