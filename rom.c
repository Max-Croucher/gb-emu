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

bool MBANK1_mode = 0;
uint8_t MBANK1_reg_BANK1 = 1;
uint8_t MBANK1_reg_BANK2 = 0;
bool MBANK1_RAMG = 0;
bool MBANK1_is_MBC1M = 0;


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
    if (romcode > 7) print_error("Unable to decode rom size.");
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
        break;
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
    if (rom.ramsize) {
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
		print_error("File is empty.");
	}
	
    init_rom(romfile);

        //load rom into memory
    fseek(romfile, 0, SEEK_SET);
    int bytes_read = fread(rom.rom, 1, rom.romsize, romfile);
    if (bytes_read != rom.romsize) print_error("Unable to load rom file.");

    fprintf(stderr,"Successfully loaded rom file.\n");
	fclose(romfile);
    detect_multicart(); // determine if the rom contains a multicart
    if (MBANK1_is_MBC1M) fprintf(stderr, "This ROM is a MBC1B Multicart\n");
}


void init_ram() {
    /* Initialise the gameboy RAM, loading the first 32K of rom at 0x0000 */
    ram = malloc(0x10000);
    memcpy(ram, rom.rom, 0x8000);

    //various ram addrs
    *(ram+0xFF44) = 0x90; // lack of lcd

    //*(ram+0xFF44) = 0b11100100; // bg pallette

    // uint8_t initial_registers[128] = {
    //     0xFF, 0x00, 0x7E, 0xFF, 0xFF, 0x00, 0x00, 0xF8, // 0xFF00
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, // 0xFF08
    //     0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, // 0xFF10
    //     0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF, // 0xFF18
    //     0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, // 0xFF20
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF28
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF30
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF38
    //     0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFC, // 0xFF40
    //     0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFE, // 0xFF48
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF50
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF58
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF60
    //     0xC8, 0xFF, 0xD0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0xFF68
    //     0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x8F, 0x00, 0x00, // 0xFF70
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF // 0xFF78
    // };
    // memcpy(ram+0xFF00, &initial_registers, 128);
}


void mbank_register(uint16_t addr, uint8_t byte) {
    /* Handle writing to an mbank register */
    //fprintf(stderr, "Received write with address 0x%.4x and value 0x%.2x\n", addr, byte);
    switch (rom.carttype)
    {
    case 0: // no MBC
        break;
    case 1: // MBC1
    case 2: // MBC1 + RAM
    case 3: // MBC1 + RAM + BATTERY
        if (addr < 0x2000) { // RAM enable
            if (rom.carttype == 2 || rom.carttype == 3) MBANK1_RAMG = ((byte&0xF) == 0xA);
            //fprintf(stderr, "RAM enable set to %s.\n", (byte == 0xA) ? "ON" : "OFF");
        } else if (addr < 0x4000) { // ROM bank switch
            MBANK1_reg_BANK1 = byte & 0x1F;
            if (MBANK1_reg_BANK1 == 0) MBANK1_reg_BANK1 = 1;
        } else if (addr < 0x6000) { // RAM bank switch and upper bits of ROM bank
            MBANK1_reg_BANK2 = byte&3;
        } else { // ROM banking mode select
            MBANK1_mode = byte&1;
        }
        break;
    default:
        print_error("MBANK type is not recognised or not supported!");
    }
}


void write_ext_ram(uint16_t addr, uint8_t byte) {
    /* Write to external cartridge RAM. Addr is normalised to 0 */
    if (MBANK1_RAMG) {
        addr &= 0x0FFF; // Truncate to bits 12-0
        addr |= (MBANK1_reg_BANK2*MBANK1_mode)<<13; // Add MBANK RAM id to bits 14-13 if mode select is 1
        addr &= (rom.ramsize-1); // Truncate to ram size
        *(rom.ram + addr) = byte;
    }
}


uint8_t read_ext_ram(uint16_t addr) {
    /* Read to external cartridge RAM. Addr is normalised to 0 */
    if (MBANK1_RAMG) {
        addr &= 0x0FFF; // Truncate to bits 12-0
        addr += (MBANK1_reg_BANK2*MBANK1_mode)<<13; // Include MBANK RAM id to bits 14-13 if mode select is 1
        addr &= (rom.ramsize-1); // Truncate to ram size
        return *(rom.ram + addr);
    } else {
        return 0xFF; // RAM is disabled
    }
}


uint8_t read_rom(uint32_t addr) {
    /* Read from ROM, factoring in bank switching. Addr normalisation is irrelevant */
    if (!MBANK1_is_MBC1M) { // NOT a multicart
        if ((addr >> 14)&1) { // reading from 0x4000-0x7FFF
            addr &= 0x3FFF; // Truncate to bits 13-0
            addr |= (MBANK1_reg_BANK1<<14); // Include MBANK ROM id to bits 18-14
            addr |= (MBANK1_reg_BANK2<<19); // Include MBANK RAM id to bits 20-19
            addr &= (rom.romsize-1); // Truncate to rom size
            return *(rom.rom + addr);
        } else { // reading from 0x0000-0x3FFF
            addr &= 0x3FFF; // Truncate to bits 13-0
            // DO NOT Include MBANK ROM id at bits 18-14
            addr |= ((MBANK1_reg_BANK2*MBANK1_mode)<<19); // Include MBANK RAM id to bits 20-19 if mode select is 1
            addr &= (rom.romsize-1); // Truncate to rom size
            return *(rom.rom + addr);
        }
    } else { // A multicart
        if ((addr >> 14)&1) { // reading from 0x4000-0x7FFF
            addr &= 0x3FFF; // Truncate to bits 13-0
            addr |= ((MBANK1_reg_BANK1&0xF)<<14); // Include MBANK ROM id to bits 17-14. Note in MBC1M mode, this is only 4 bits long
            addr |= (MBANK1_reg_BANK2<<18); // Include MBANK RAM id to bits 19-18
            addr &= (rom.romsize-1); // Truncate to rom size
            return *(rom.rom + addr);
        } else { // reading from 0x0000-0x3FFF
            addr &= 0x3FFF; // Truncate to bits 13-0
            // DO NOT Include MBANK ROM id at bits 17-14
            addr |= ((MBANK1_reg_BANK2*MBANK1_mode)<<18); // Include MBANK RAM id to bits 19-18 if mode select is 1
            addr &= (rom.romsize-1); // Truncate to rom size
            return *(rom.rom + addr);
        }
    }
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