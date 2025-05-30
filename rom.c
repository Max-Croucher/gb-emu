/* Source file for rom.c, controlling the reading and handling of a gameboy rom file
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "rom.h"

gbRom rom; //extern
extern uint8_t* ram;

static bool MBANK_mode = 0;
static uint16_t MBANK_reg_BANK1 = 1;
static uint8_t MBANK_reg_BANK2 = 0;
static bool MBANK_RAMG = 0;
static bool MBANK1_is_MBC1M = 0;
// static bool MBANK3_RTC_latch = 1;
// static bool MBANK3_RTC_last_latch = 1;
// static uint8_t MBANK3_RTC_S = 0;
// static uint8_t MBANK3_RTC_M = 0;
// static uint8_t MBANK3_RTC_H = 0;
// static uint8_t MBANK3_RTC_DL = 0;
// static uint8_t MBANK3_RTC_DH = 0;
// static uint8_t MBANK3_RTC_latch_S = 0;
// static uint8_t MBANK3_RTC_latch_M = 0;
// static uint8_t MBANK3_RTC_latch_H = 0;
// static uint8_t MBANK3_RTC_latch_DL = 0;
// static uint8_t MBANK3_RTC_latch_DH = 0;


void (*write_MBANK_register)(uint16_t, uint8_t); //extern
uint8_t (*read_rom)(uint32_t); //extern
void (*write_ext_ram)(uint16_t, uint8_t); //extern
uint8_t (*read_ext_ram)(uint16_t); //extern




void print_error(char errormsg[]) {
    /* print an error message and exit */
    fprintf(stderr, "Error: %s\n", errormsg);
    exit(EXIT_FAILURE);
}


void print_header() {
    /* print the contents of a gameboy header */
    fprintf(stderr,"Header contents:\n");
    fprintf(stderr,"Title:          %s\n", rom.title);
    fprintf(stderr,"Licensee (new): %c%c\n", rom.new_licensee>>8, rom.new_licensee&0xFF);
    fprintf(stderr,"Super Gameboy:  %d\n", rom.sgbflag);
    fprintf(stderr,"Cartrige Type:  %d\n", rom.carttype);
    fprintf(stderr,"ROM Size Code:  %d\n", rom.romsize);
    fprintf(stderr,"RAM Size Code:  %d\n", rom.ramsize);
    fprintf(stderr,"Global Release: 0x%.2x\n", rom.locale);
    fprintf(stderr,"Licensee (old): 0x%.2x\n", rom.old_licensee);
    fprintf(stderr,"Version:        %d\n", rom.version);
}


int decode_rom_size(uint8_t romcode) {
    /* Decode the rom size indicated in a gameboy header, returns the number of bytes */
    if (romcode > 8) print_error("Unable to decode rom size.");
    return 1<<(romcode+15); //0 means 32K, 1 means 64k and so on.
}


int decode_ram_size(uint8_t ramcode) {
    /* Decode the ram size indicated in a gameboy header, returns the number of bytes */
    switch (ramcode)
    {
    case 0:
        return 0;
    
    case 1:
        return 1<<11; //2K
    
    case 2:
        return 1<<13; //8K
    
    case 3:
        return 1<<15; //32K
    
    default:
        print_error("Unable to decode ram size.");
        return 0;
    }
}


void init_rom(FILE* romfile) {
    /* read a gameboy rom to process the header and load the rom contents into memory */
    int read_errors = 0;
    fseek(romfile, HEADER_START, SEEK_SET);
    read_errors += 16!=fread(&rom.title, 1, 16, romfile);
    read_errors += 1!=fread(&rom.new_licensee, 2, 1, romfile);

    uint8_t sgbflag;
    read_errors += 1!=fread(&sgbflag, 1, 1, romfile);
    rom.sgbflag = sgbflag==0x03;

    read_errors += 1!=fread(&rom.carttype, 1, 1, romfile);

    uint8_t romcode;
    uint8_t ramcode;
    read_errors += 1!=fread(&romcode, 1, 1, romfile);
    read_errors += 1!=fread(&ramcode, 1, 1, romfile);
    rom.romsize = decode_rom_size(romcode);
    rom.ramsize = decode_ram_size(ramcode);

    rom.rom = malloc(rom.romsize);
    if (rom.carttype == 5 || rom.carttype == 6) {
        rom.ram = malloc(0x0200);
    } else if (rom.ramsize) {
        rom.ram = malloc(rom.ramsize);
    } else {
        rom.ram = 0;
    }

    read_errors += 1!=fread(&rom.locale, 1, 1, romfile);
    read_errors += 1!=fread(&rom.old_licensee, 1, 1, romfile);
    read_errors += 1!=fread(&rom.version, 1, 1, romfile);
    uint8_t checksum;
    read_errors += 1!=fread(&checksum, 1, 1, romfile);
    if (read_errors) print_error("Unable to read header: EOF.");

    //Verify Checksum
    uint8_t raw_header[HEADER_SIZE];
    fseek(romfile, HEADER_START, SEEK_SET);
    fread(raw_header, 1, HEADER_SIZE, romfile);
    for (uint8_t i=0; i<HEADER_SIZE; i++) {
        checksum += raw_header[i]+1;
    }
    if (checksum) print_error("Header Checksum is invalid!");

    print_header();
}


void load_rom(char filename[]) {
	/* Read a rom file and load contents */
    fprintf(stderr,"Attempting to load rom file %s\n", filename);
	FILE *romfile;
	romfile = fopen(filename, "rb");
	if (romfile == NULL) {
		print_error("Unable to open ROM file.");
	}
	
    init_rom(romfile);

        //load rom into memory
    fseek(romfile, 0, SEEK_SET);
    int bytes_read = fread(rom.rom, 1, rom.romsize, romfile);
    if (bytes_read != rom.romsize) print_error("Unable to load rom file.");

    fprintf(stderr,"Successfully loaded rom file.\n");
	fclose(romfile);
    initialise_rom_address_functions();
}


void init_ram() {
    /* Initialise the gameboy RAM, loading the first 32K of rom at 0x0000 */
    ram = malloc(0x10000);
    memcpy(ram, rom.rom, 0x8000);

    //various ram addrs
    //*(ram+0xFF00) = 0xFF; // JOYP
    //*(ram+0xFF01) = 0x3F; // serial stub
    //*(ram+0xFF01) = 0xFF; // SC
    //*(ram+0xFF40) = 0x91; // lcdc init
    //*(ram+0xFF44) = 0x90; // lcd stub

    //*(ram+0xFF47) = 0b11100100; // bg pallette
    uint8_t initial_registers[256] = {
        0xcf, 0x00, 0x7e, 0xff, 0xab, 0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe1, // 0x00
        0x80, 0xbf, 0xf3, 0xff, 0xbf, 0xff, 0x3f, 0x00, 0xff, 0xbf, 0x7f, 0xff, 0x9f, 0xff, 0xbf, 0xff, // 0x01
        0xff, 0x00, 0x00, 0xbf, 0x77, 0xf3, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x02
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x03
        0x91, 0x85, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfc, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, // 0x04
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x05
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x06
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x07
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x08
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x09
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x0a
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x0b
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x0c
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x0d
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 0x0e
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00  // 0x0f
    };
    memcpy(ram+0xFF00, &initial_registers, 256);
}


void detect_multicart(void) {
    /* if the ROM uses MBC1, check if the game contains a multicart by verifying
    at least three of the first four banks contains the NINTENDO logo checksum
    (i.e. 0x0104-0x0133 has the appropriate checksum) */
    if (rom.carttype == 0 || rom.carttype > 3) return; // not MBC1
    if (rom.romsize != 1<<20) return; // not 1Mb
    uint8_t num_successes = 0;
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t checksum = 0x46;
        for (uint32_t addr = 0x4000*i + 0x0104; addr < 0x4000*i + 0x0134; addr++) {
            checksum -= *(rom.rom + addr);
        }
        if (!checksum) num_successes++;
    }
    MBANK1_is_MBC1M = (num_successes>2);
}


void initialise_rom_address_functions(void) {
    /* Detect the ROM's MBANK type and set the functions write_MBANK_register, 
    write_ext_ram, read_ext_ram and read_rom to the appropriate versions */
    switch (rom.carttype)
    {
    case 0x00: // No MBC
        write_MBANK_register = &_NO_MBC_write_MBANK_register;
        read_rom = &_NO_MBC_read_rom;
        write_ext_ram = &_NO_MBC_write_ext_ram;
        read_ext_ram = & _NO_MBC_read_ext_ram;
        break;
    case 0x01:
    case 0x02:
    case 0x03: // MBC1 or MBC1M
        write_MBANK_register = &_MBC1_write_MBANK_register;
        read_rom = &_MBC1_read_rom;
        write_ext_ram = &_MBC1_write_ext_ram;
        read_ext_ram = & _MBC1_read_ext_ram;
        detect_multicart(); // determine if the rom contains a multicart
        if (MBANK1_is_MBC1M) read_rom = &_MBC1_MULTICART_read_rom;
        break;
    case 0x05:
    case 0x06: // MBC2
        write_MBANK_register = &_MBC2_write_MBANK_register;
        read_rom = &_MBC2_read_rom;
        write_ext_ram = &_MBC2_write_ext_ram;
        read_ext_ram = & _MBC2_read_ext_ram;
        break;
    // case 0x0F:
    // case 0x10:
    // case 0x11:
    // case 0x12:
    // case 0x13: // MBC3
    //     write_MBANK_register = &_MBC3_write_MBANK_register;
    //     read_rom = &_MBC3_read_rom;
    //     write_ext_ram = &_MBC3_write_ext_ram;
    //     read_ext_ram = & _MBC3_read_ext_ram;
    //     break;
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E: // MBC5
        write_MBANK_register = &_MBC5_write_MBANK_register;
        read_rom = &_MBC5_read_rom;
        write_ext_ram = &_MBC5_write_ext_ram;
        read_ext_ram = & _MBC5_read_ext_ram;
        break;
    default:
        print_error("MBANK type is not recognised or not supported!");
    }
}


// void freeze_mbank3_rtc(void) {
//     /* latch the current states of MBANK3 RTC registers */
//     MBANK3_RTC_latch_S = MBANK3_RTC_S;
//     MBANK3_RTC_latch_M = MBANK3_RTC_M;
//     MBANK3_RTC_latch_H = MBANK3_RTC_H;
//     MBANK3_RTC_latch_DL = MBANK3_RTC_DL;
//     MBANK3_RTC_latch_DH = MBANK3_RTC_DH;
// }

static void _NO_MBC_write_MBANK_register(uint16_t mbc_reg, uint8_t byte) {
    /* Handle writing to the ROM with no MBANK. ignore */
}


static uint8_t _NO_MBC_read_rom(uint32_t addr) {
    /* Read from ROM, without switching */
    return *(rom.rom + addr);
}


static void _NO_MBC_write_ext_ram(uint16_t addr, uint8_t byte) {
    /* Handle writing to nonexistent RAM */
}


static uint8_t _NO_MBC_read_ext_ram(uint16_t addr) {
    /* Handle reading from nonexistent RAM */
    return 0xFF;
}


static void _MBC1_write_MBANK_register(uint16_t addr, uint8_t byte) {
    /* Handle writing to an MBANK1 register */
    switch (addr >> 13)
    {
    case 0: // 0x0000-0x1FFF RAM enable
        if ((rom.carttype == 2 || rom.carttype == 3) && rom.ramsize) MBANK_RAMG = ((byte&0xF) == 0xA);
        break;
        //fprintf(stderr, "RAM enable set to %s.\n", (byte == 0xA) ? "ON" : "OFF");
    case 1: // 0x2000-0x3FFF ROM bank switch
        MBANK_reg_BANK1 = byte & 0x1F;
        if (MBANK_reg_BANK1 == 0) MBANK_reg_BANK1 = 1;
        break;
    case 2: // 0x4000-0x5FFF RAM bank switch and upper bits of ROM bank
        MBANK_reg_BANK2 = byte&3;
        break;
    default:
        MBANK_mode = byte&1;
        break;
    }
}


static uint8_t _MBC1_read_rom(uint32_t addr) {
    /* Read from ROM, factoring in MBANK1 bank switching. Addr normalisation is irrelevant */
    if ((addr >> 14)&1) { // reading from 0x4000-0x7FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        addr |= (MBANK_reg_BANK1<<14); // Include MBANK ROM id to bits 18-14
        addr |= (MBANK_reg_BANK2<<19); // Include MBANK RAM id to bits 20-19
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    } else { // reading from 0x0000-0x3FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        // DO NOT Include MBANK ROM id at bits 18-14
        addr |= ((MBANK_reg_BANK2*MBANK_mode)<<19); // Include MBANK RAM id to bits 20-19 if mode select is 1
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    }
}


static uint8_t _MBC1_MULTICART_read_rom(uint32_t addr) {
    /* Read from ROM, factoring in MBANK1 Multicart bank switching. Addr normalisation is irrelevant */
    if ((addr >> 14)&1) { // reading from 0x4000-0x7FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        addr |= ((MBANK_reg_BANK1&0xF)<<14); // Include MBANK ROM id to bits 17-14. Note in MBC1M mode, this is only 4 bits long
        addr |= (MBANK_reg_BANK2<<18); // Include MBANK RAM id to bits 19-18
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    } else { // reading from 0x0000-0x3FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        // DO NOT Include MBANK ROM id at bits 17-14
        addr |= ((MBANK_reg_BANK2*MBANK_mode)<<18); // Include MBANK RAM id to bits 19-18 if mode select is 1
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    }
}


static void _MBC1_write_ext_ram(uint16_t addr, uint8_t byte) {
    /* Write to external MBANK1 RAM. Addr is normalised to 0 */
    if (MBANK_RAMG) {
        addr &= 0x0FFF; // Truncate to bits 12-0
        addr |= (MBANK_reg_BANK2*MBANK_mode)<<13; // Add MBANK RAM id to bits 14-13 if mode select is 1
        addr &= (rom.ramsize-1); // Truncate to ram size
        *(rom.ram + addr) = byte;
    }
}


static uint8_t _MBC1_read_ext_ram(uint16_t addr) {
    /* Read from external MBANK1 RAM. Addr is normalised to 0 */
    if (MBANK_RAMG) {
        addr &= 0x0FFF; // Truncate to bits 12-0
        addr += (MBANK_reg_BANK2*MBANK_mode)<<13; // Include MBANK RAM id to bits 14-13 if mode select is 1
        addr &= (rom.ramsize-1); // Truncate to ram size
        return *(rom.ram + addr);
    } else {
        return 0xFF; // RAM is disabled
    }
}


static void _MBC2_write_MBANK_register(uint16_t addr, uint8_t byte) {
    /* Handle writing to an MBANK2 register */
    if (addr < 0x4000) {
        if (addr&0x0100) { // If bit 8 is set, control ROM bank
            MBANK_reg_BANK1 = byte&0xF;
            if (MBANK_reg_BANK1 == 0) MBANK_reg_BANK1 = 1;
        } else { // conrol RAMG
            MBANK_RAMG = ((byte&0xF) == 0xA);
        }
    }
}


static uint8_t _MBC2_read_rom(uint32_t addr) {
    /* Read from ROM, factoring in MBANK2 bank switching. Addr normalisation is irrelevant */
    if ((addr >> 14)&1) { // reading from 0x4000-0x7FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        addr |= ((MBANK_reg_BANK1&0xF)<<14); // Include MBANK ROM id to bits 17-14. this is only 4 bits long
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    } else { // reading from 0x0000-0x3FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    }
}


static void _MBC2_write_ext_ram(uint16_t addr, uint8_t byte) {
    /* Write to external MBANK2 RAM. Addr is normalised to 0 */
    if (MBANK_RAMG) {
        addr &= 0x01FF; // Truncate to bits 8-0
        *(rom.ram + addr) = byte|0xF0; // only the lower 4 bits are usable
    }
}


uint8_t _MBC2_read_ext_ram(uint16_t addr) {
    /* Read from external MBANK2 RAM. Addr is normalised to 0 */
    if (MBANK_RAMG) {
        addr &= 0x01FF; // Truncate to bits 8-0
        return *(rom.ram + addr)|0xF0; // only the lower 4 bits are usable
    }
    return 0xFF;
}


// static void _MBC3_write_MBANK_register(uint16_t addr, uint8_t byte) {
//     /* Handle writing to an MBANK3 register */
//     switch (addr >> 13)
//     {
//     case 0: // 0x0000-0x1FFF RAM enable
//         if (rom.carttype == 12 || rom.carttype == 13) MBANK_RAMG = ((byte&0xF) == 0x0A);
//         break;
//     case 1: // 0x2000-0x3FFF ROM bank switch
//         MBANK_reg_BANK1 = byte & 0x7F;
//         if (MBANK_reg_BANK1 == 0) MBANK_reg_BANK1 = 1;
//         break;
//     case 2: // 0x4000-0x5FFF RAM bank switch or RTC register
//         MBANK_reg_BANK2 = byte&0xF;
//         break;
//     case 3: // 0x6000-0x7FFF RTC Latch - Freeze RTC registers for reading
//         MBANK3_RTC_last_latch = MBANK3_RTC_latch;
//         MBANK3_RTC_latch = byte&1;
//         if (MBANK3_RTC_latch && !MBANK3_RTC_last_latch) freeze_mbank3_rtc(); // Freeze clock if bit 0 goes high
//     default:
//         MBANK_mode = byte&1;
//         break;
//     }
// }


// static uint8_t _MBC3_read_rom(uint32_t addr) {
//     /* Read from ROM, factoring in MBANK3 bank switching. Addr normalisation is irrelevant */
//     if ((addr >> 14)&1) { // reading from 0x4000-0x7FFF
//         addr &= 0x3FFF; // Truncate to bits 13-0
//         addr |= (MBANK_reg_BANK1<<14); // Include MBANK ROM id to bits 21-14.
//         addr &= (rom.romsize-1); // Truncate to rom size
//         return *(rom.rom + addr);
//     } else { // reading from 0x0000-0x3FFF
//         addr &= 0x3FFF; // Truncate to bits 13-0
//         addr &= (rom.romsize-1); // Truncate to rom size
//         return *(rom.rom + addr);
//     }
// }


// static void _MBC3_write_ext_ram(uint16_t addr, uint8_t byte) {
//     /* Write to external MBANK3 RAM or RTC registers. Addr is normalised to 0 */
//     if (MBANK_RAMG) {
//         if (MBANK_reg_BANK2 < 7) {
//             addr &= 0x01FFF; // Truncate to bits 12-0
//             addr += (MBANK_reg_BANK2&3)<<13; // Include MBANK RAM id
//             addr &= (rom.ramsize-1); // Truncate to ram size
//             *(rom.ram + addr) = byte;
//         } else if (MBANK_reg_BANK2 == 8) {
//             MBANK3_RTC_latch_S = byte;
//             MBANK3_RTC_S = byte;
//         } else if (MBANK_reg_BANK2 == 9) {
//             MBANK3_RTC_latch_M = byte;
//             MBANK3_RTC_M = byte;
//         } else if (MBANK_reg_BANK2 == 0xA) {
//             MBANK3_RTC_latch_H = byte;
//             MBANK3_RTC_H = byte;
//         } else if (MBANK_reg_BANK2 == 0xB) {
//             MBANK3_RTC_latch_DL = byte;
//             MBANK3_RTC_DL = byte;
//         } else if (MBANK_reg_BANK2 == 0xC) {
//             MBANK3_RTC_latch_DL = byte;
//             MBANK3_RTC_DL = byte;
//         }
//     }
// }


// uint8_t _MBC3_read_ext_ram(uint16_t addr) {
//     /* Read from external MBANK3 RAM or latched RTC registers. Addr is normalised to 0 */
//     if (MBANK_RAMG) {
//         if (MBANK_reg_BANK2 < 7) {
//             addr &= 0x01FFF; // Truncate to bits 12-0
//             addr += (MBANK_reg_BANK2&3)<<13; // Include MBANK RAM id
//             addr &= (rom.ramsize-1); // Truncate to ram size
//             return *(rom.ram + addr);
//         } else if (MBANK_reg_BANK2 == 8) {
//             return MBANK3_RTC_latch_S;
//         } else if (MBANK_reg_BANK2 == 9) {
//             return MBANK3_RTC_latch_M;
//         } else if (MBANK_reg_BANK2 == 0xA) {
//             return MBANK3_RTC_latch_H;
//         } else if (MBANK_reg_BANK2 == 0xB) {
//             return MBANK3_RTC_latch_DL;
//         } else if (MBANK_reg_BANK2 == 0xC) {
//             return MBANK3_RTC_latch_DH;
//         }
//     }
//     return 0xFF;
// }


static void _MBC5_write_MBANK_register(uint16_t addr, uint8_t byte) {
    /* Handle writing to an MBANK5 register */
    switch (addr >> 12)
    {
    case 0:
    case 1: // 0x0000-0x1FFF RAM enable
        if ((rom.carttype == 0x1A || rom.carttype == 0x1B || rom.carttype == 0x1D || rom.carttype == 0x1E) && rom.ramsize) MBANK_RAMG = ((byte&0xF) == 0xA);
        break;
    case 2: // 0x2000-0x2FFF ROM bank switch lower 8 bits
        MBANK_reg_BANK1 &= 0x0100;
        MBANK_reg_BANK1 |= byte;
        break;
    case 3: // 0x3000-0x3FFF ROM bank switch bit 8
        MBANK_reg_BANK1 &= 0x00FF;
        MBANK_reg_BANK1 |= (byte&1)<<8;
        break;
    case 4: // 0x4000-0x4FFF RAM bank switch
        MBANK_reg_BANK2 = byte&0xF;
        break;
    default:
        break;
    }
}


static uint8_t _MBC5_read_rom(uint32_t addr) {
    /* Read from ROM, factoring in MBANK5 bank switching. Addr normalisation is irrelevant */
    if ((addr >> 14)&1) { // reading from 0x4000-0x7FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        addr |= MBANK_reg_BANK1<<14; // Include MBANK ROM id to bits 22-14. this is 9 bits long
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    } else { // reading from 0x0000-0x3FFF
        addr &= 0x3FFF; // Truncate to bits 13-0
        addr &= (rom.romsize-1); // Truncate to rom size
        return *(rom.rom + addr);
    }
}


static void _MBC5_write_ext_ram(uint16_t addr, uint8_t byte) {
    /* Write to external MBANK5 RAM. Addr is normalised to 0 */
    if (MBANK_RAMG) {
        addr &= 0x0FFF; // Truncate to bit 12
        addr += (MBANK_reg_BANK2&0xF)<<13;
        *(rom.ram + addr) = byte;
    }
}


uint8_t _MBC5_read_ext_ram(uint16_t addr) {
    /* Read from external MBANK5 RAM. Addr is normalised to 0 */
    if (MBANK_RAMG) {
        addr &= 0x0FFF; // Truncate to bit 12
        addr += (MBANK_reg_BANK2&0xF)<<13;
        return *(rom.ram + addr);
    }
    return 0xFF;
}