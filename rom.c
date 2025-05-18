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

static gbRom rom_container;

void print_error(char errormsg[]) {
    /* print an error message and exit */
    fprintf(stderr, "Error: %s\n", errormsg);
    exit(EXIT_FAILURE);
}


void print_header(gbRom rom_container) {
    /* print the contents of a gameboy header */
    fprintf(stderr,"Header contents:\n");
    fprintf(stderr,"Title:          %s\n", rom_container.title);
    fprintf(stderr,"Licensee (new): %d\n", rom_container.new_licensee);
    fprintf(stderr,"Super Gameboy:  %d\n", rom_container.sgbflag);
    fprintf(stderr,"Cartrige Type:  %d\n", rom_container.carttype);
    fprintf(stderr,"ROM Size Code:  %d\n", rom_container.romsize);
    fprintf(stderr,"RAM Size Code:  %d\n", rom_container.ramsize);
    fprintf(stderr,"Global Release: %d\n", rom_container.locale);
    fprintf(stderr,"Licensee (old): %d\n", rom_container.old_licensee);
    fprintf(stderr,"Version:        %d\n", rom_container.version);
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


gbRom init_rom(FILE* romfile) {
    /* read a gameboy rom to process the header and load the rom contents into memory */
    int read_errors = 0;
    fseek(romfile, HEADER_START, SEEK_SET);
    read_errors += 16!=fread(&rom_container.title, 1, 16, romfile);
    read_errors += 1!=fread(&rom_container.new_licensee, 2, 1, romfile);

    uint8_t sgbflag;
    read_errors += 1!=fread(&sgbflag, 1, 1, romfile);
    rom_container.sgbflag = sgbflag==0x03;

    read_errors += 1!=fread(&rom_container.carttype, 1, 1, romfile);

    uint8_t romcode;
    uint8_t ramcode;
    read_errors += 1!=fread(&romcode, 1, 1, romfile);
    read_errors += 1!=fread(&ramcode, 1, 1, romfile);
    rom_container.romsize = decode_rom_size(romcode);
    rom_container.ramsize = decode_ram_size(ramcode);

    rom_container.rom = malloc(rom_container.romsize);
    if (rom_container.ramsize) {
        rom_container.ram = malloc(rom_container.ramsize);
    } else {
        rom_container.ram = 0;
    }

    read_errors += 1!=fread(&rom_container.locale, 1, 1, romfile);
    read_errors += 1!=fread(&rom_container.old_licensee, 1, 1, romfile);
    read_errors += 1!=fread(&rom_container.version, 1, 1, romfile);
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

    print_header(rom_container);
    return rom_container;
}


gbRom load_rom(char filename[]) {
	/* Read a rom file and load contents */
    fprintf(stderr,"Attempting to load rom file %s\n", filename);
	FILE *romfile;
	romfile = fopen(filename, "rb");
	if (romfile == NULL) {
		print_error("File is empty.");
	}
	
    gbRom rom_container = init_rom(romfile);

        //load rom into memory
    fseek(romfile, 0, SEEK_SET);
    int bytes_read = fread(rom_container.rom, 1, rom_container.romsize, romfile);
    if (bytes_read != rom_container.romsize) print_error("Unable to load rom file.");

    fprintf(stderr,"Successfully loaded rom file.\n");

	fclose(romfile);
    return rom_container;
}


uint8_t* init_ram(gbRom *rom) {
    /* Initialise the gameboy RAM, loading the first 32K of rom at 0x0000 */
    uint8_t *ram = malloc(0x10000);
    memcpy(ram, (*rom).rom, 0x8000);

    //various ram addrs
    *(ram+0xFF44) = 0x90;

    return ram;
}


void mbank_register(uint8_t* ram, uint8_t mbc_reg, uint8_t value) {
    /* Handle writing to an mbank register */
    if (rom_container.carttype == 1) {
        if (mbc_reg = 1) {
            value &= 0x1F;
            if (value == 0) value += 1;
            uint8_t num_banks = rom_container.romsize>>15;
            uint8_t bank_id = value&(num_banks-1);
            memcpy(ram+0x4000, rom_container.rom+(bank_id*0x4000), 0x4000);
        }
    }
}