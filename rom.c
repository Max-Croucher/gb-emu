/* Source file for rom.c, controlling the reading and handling of a gameboy rom file
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "rom.h"

gbRom rom; //extern
extern uint8_t* ram;

void print_error(char errormsg[]) {
    /* print an error message and exit */
    fprintf(stderr, "Error: %s\n", errormsg);
    exit(EXIT_FAILURE);
}


void print_header() {
    /* print the contents of a gameboy header */
    fprintf(stderr,"Header contents:\n");
    fprintf(stderr,"Title:          %s\n", rom.title);
    fprintf(stderr,"Licensee (new): %d\n", rom.new_licensee);
    fprintf(stderr,"Super Gameboy:  %d\n", rom.sgbflag);
    fprintf(stderr,"Cartrige Type:  %d\n", rom.carttype);
    fprintf(stderr,"ROM Size Code:  %d\n", rom.romsize);
    fprintf(stderr,"RAM Size Code:  %d\n", rom.ramsize);
    fprintf(stderr,"Global Release: %d\n", rom.locale);
    fprintf(stderr,"Licensee (old): %d\n", rom.old_licensee);
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

void mbank_register(uint8_t mbc_reg, uint8_t value) {
    /* Handle writing to an mbank register */
    if (rom.carttype == 1) {
        if (mbc_reg = 1) {
            value &= 0x1F;
            if (value == 0) value += 1;
            uint8_t num_banks = rom.romsize>>15;
            uint8_t bank_id = value&(num_banks-1);
            memcpy(ram+0x4000, rom.rom+(bank_id*0x4000), 0x4000);
        }
    }
}