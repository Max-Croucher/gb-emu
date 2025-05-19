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
void mbank_register(uint8_t mbc_reg, uint8_t value);

#endif // ROM_H