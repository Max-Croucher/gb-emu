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

static char *MBANK_NAMES[] = {
  "ROM",
  "MBANK1",
  "MBANK2",
  "MBANK3",
  "MBANK4",
  "MBANK5",
  "MBANK6",
  "MBANK7",
  "MMM01",
  "CAMERA",
  "TAMA5",
  "HUC3",
  "HUC1",
  "MBC1M"
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

static void _MBC3_write_MBANK_register(uint16_t mbc_reg, uint8_t byte);
static uint8_t _MBC3_read_rom(uint32_t addr);
static void _MBC3_write_ext_ram(uint16_t addr, uint8_t byte);
static uint8_t _MBC3_read_ext_ram(uint16_t addr);

static void _MBC5_write_MBANK_register(uint16_t mbc_reg, uint8_t byte);
static uint8_t _MBC5_read_rom(uint32_t addr);
static void _MBC5_write_ext_ram(uint16_t addr, uint8_t byte);
static uint8_t _MBC5_read_ext_ram(uint16_t addr);

#endif // ROM_H