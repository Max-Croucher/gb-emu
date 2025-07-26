/* Header file for cpu.c, controlling the execution of the gameboy cpu
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef CPU_H
#define CPU_H

#define CLK_HZ 4194304UL
#define PROG_START 0x0100

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


typedef enum registers{
    REG_JOYP   = 0xff00,
    REG_SB     = 0xff01,
    REG_SC     = 0xff02,
    REG_DIV    = 0xff04,
    REG_TIMA   = 0xff05,
    REG_TMA    = 0xff06,
    REG_TAC    = 0xff07,
    REG_IF     = 0xff0f,
    REG_NR10   = 0xff10,
    REG_NR11   = 0xff11,
    REG_NR12   = 0xff12,
    REG_NR13   = 0xff13,
    REG_NR14   = 0xff14,
    REG_NR21   = 0xff16,
    REG_NR22   = 0xff17,
    REG_NR23   = 0xff18,
    REG_NR24   = 0xff19,
    REG_NR30   = 0xff1a,
    REG_NR31   = 0xff1b,
    REG_NR32   = 0xff1c,
    REG_NR33   = 0xff1d,
    REG_NR34   = 0xff1e,
    REG_NR41   = 0xff20,
    REG_NR42   = 0xff21,
    REG_NR43   = 0xff22,
    REG_NR44   = 0xff23,
    REG_NR50   = 0xff24,
    REG_NR51   = 0xff25,
    REG_NR52   = 0xff26,
    REG_WAVE1  = 0xff30,
    REG_WAVE2  = 0xff31,
    REG_WAVE3  = 0xff32,
    REG_WAVE4  = 0xff33,
    REG_WAVE5  = 0xff34,
    REG_WAVE6  = 0xff35,
    REG_WAVE7  = 0xff36,
    REG_WAVE8  = 0xff37,
    REG_WAVE9  = 0xff38,
    REG_WAVE10 = 0xff39,
    REG_WAVE11 = 0xff3a,
    REG_WAVE12 = 0xff3b,
    REG_WAVE13 = 0xff3c,
    REG_WAVE14 = 0xff3d,
    REG_WAVE15 = 0xff3e,
    REG_WAVE16 = 0xff3f,
    REG_LCDC   = 0xff40,
    REG_STAT   = 0xff41,
    REG_SCY    = 0xff42,
    REG_SCX    = 0xff43,
    REG_LY     = 0xff44,
    REG_LYC    = 0xff45,
    REG_DMA    = 0xff46,
    REG_BGP    = 0xff47,
    REG_OBP0   = 0xff48,
    REG_OBP1   = 0xff49,
    REG_WX     = 0xff4a,
    REG_WY     = 0xff4b,
    REG_IE     = 0xffff,
}registers;


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
void read_dma(void);
void joypad_io(void);

#endif // CPU_H