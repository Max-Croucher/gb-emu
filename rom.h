/* Header file for rom.c, controlling the reading and handling of a gameboy rom file
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef ROM_H
#define ROM_H

#define HEADER_START 0x0134
#define HEADER_SIZE 25

enum MBCType {
  MBANK_NONE=0,
  MBANK_1,
  MBANK_2,
  MBANK_3,
  MBANK_4,
  MBANK_5,
  MBANK_6,
  MBANK_7,
  MBANK_MMMO1,
  MBANK_CAMERA,
  MBANK_TAMA5,
  MBANK_HUC3,
  MBANK_HUC1,
  MBANK_1_MULTICART
};

typedef struct {
    uint8_t sgbflag;
    uint8_t mbc_type;
    uint8_t* rom;
    uint8_t* ram;
    uint8_t locale;
    uint8_t old_licensee;
    uint8_t version;
    uint8_t cart_type_id;
    bool has_battery;
    bool has_timer;
    uint16_t new_licensee;
    int ramsize;
    int romsize;
    char title[16];
} gbRom;

void init_rom(FILE* romfile);
void print_header(void);
void print_error(char errormsg[]);
int decode_rom_size(uint8_t romcode);
int decode_ram_size(uint8_t ramcode);
bool detect_multicart(void);
void decode_cartridge_type(void);
void load_rom(char filename[]);
void init_ram(void);
void initialise_rom_address_functions(void);
void freeze_mbank3_rtc(void);

#endif // ROM_H