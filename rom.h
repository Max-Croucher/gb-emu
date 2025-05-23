/* Header file for rom.c, controlling the reading and handling of a gameboy rom file
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef ROM_H
#define ROM_H

#define HEADER_START 0x0134
#define HEADER_SIZE 25

typedef struct {
    char title[16];
    uint16_t new_licensee;
    uint8_t sgbflag;
    uint8_t carttype;
    int romsize;
    uint8_t* rom;
    int ramsize;
    uint8_t* ram;
    uint8_t locale;
    uint8_t old_licensee;
    uint8_t version;
} gbRom;

void init_rom(FILE* romfile);
void print_header(void);
void print_error(char errormsg[]);
int decode_rom_size(uint8_t romcode);
int decode_ram_size(uint8_t ramcode);
void load_rom(char filename[]);
void init_ram(void);
// void write_MBANK_register(uint16_t mbc_reg, uint8_t byte);
// uint8_t read_rom(uint32_t addr);
// void write_ext_ram(uint16_t addr, uint8_t byte);
// uint8_t read_ext_ram(uint16_t addr);
void detect_multicart(void);
void initialise_rom_address_functions(void);

//MBANK i/o functions
static void _NO_MBC_write_MBANK_register(uint16_t mbc_reg, uint8_t byte);
static uint8_t _NO_MBC_read_rom(uint32_t addr);
static void _NO_MBC_write_ext_ram(uint16_t addr, uint8_t byte);
static uint8_t _NO_MBC_read_ext_ram(uint16_t addr);

static void _MBC1_write_MBANK_register(uint16_t mbc_reg, uint8_t byte);
static uint8_t _MBC1_read_rom(uint32_t addr);
static uint8_t _MBC1_MULTICART_read_rom(uint32_t addr);
static void _MBC1_write_ext_ram(uint16_t addr, uint8_t byte);
static uint8_t _MBC1_read_ext_ram(uint16_t addr);

static void _MBC2_write_MBANK_register(uint16_t mbc_reg, uint8_t byte);
static uint8_t _MBC2_read_rom(uint32_t addr);
static void _MBC2_write_ext_ram(uint16_t addr, uint8_t byte);
static uint8_t _MBC2_read_ext_ram(uint16_t addr);

#endif // ROM_H