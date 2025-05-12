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

void print_error(char errormsg[]) {
    /* print an error message and exit */
    fprintf(stderr, "Error: %s\n", errormsg);
    exit(EXIT_FAILURE);
}

void print_header(gbHeader header) {
    /* print the contents of a gameboy header */
    printf("Header contents:\n");
    printf("Title:          %s\n", header.title);
    printf("Licensee (new): %d\n", header.new_licensee);
    printf("Super Gameboy:  %d\n", header.sgbflag);
    printf("Cartrige Type:  %d\n", header.carttype);
    printf("ROM Size Code:  %d\n", header.romsize);
    printf("RAM Size Code:  %d\n", header.ramsize);
    printf("Global Release: %d\n", header.locale);
    printf("Licensee (old): %d\n", header.old_licensee);
    printf("Version:        %d\n", header.version);
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

gbHeader read_rom_header(FILE* romfile) {
    /* read a gameboy rom and return the rom header */
    gbHeader header;
    int read_errors = 0;
    fseek(romfile, HEADER_START, SEEK_SET);
    read_errors += 16!=fread(&header.title, 1, 16, romfile);
    read_errors += 1!=fread(&header.new_licensee, 2, 1, romfile);

    uint8_t sgbflag;
    read_errors += 1!=fread(&sgbflag, 1, 1, romfile);
    header.sgbflag = sgbflag==0x03;

    read_errors += 1!=fread(&header.carttype, 1, 1, romfile);

    uint8_t romcode;
    uint8_t ramcode;
    read_errors += 1!=fread(&romcode, 1, 1, romfile);
    read_errors += 1!=fread(&ramcode, 1, 1, romfile);
    header.romsize = decode_rom_size(romcode);
    header.ramsize = decode_ram_size(ramcode);

    read_errors += 1!=fread(&header.locale, 1, 1, romfile);
    read_errors += 1!=fread(&header.old_licensee, 1, 1, romfile);
    read_errors += 1!=fread(&header.version, 1, 1, romfile);
    uint8_t checksum;
    read_errors += 1!=fread(&checksum, 1, 1, romfile);
    if (read_errors) print_error("Unable to read header: EOF.");

    //Verify Checksum
    uint8_t raw_header[HEADER_SIZE];
    fseek(romfile, HEADER_START, SEEK_SET);
    fread(raw_header, 1, HEADER_SIZE, romfile);
    printf("%d\n",checksum);
    for (uint8_t i=0; i<HEADER_SIZE; i++) {
        checksum += raw_header[i]+1;
    }
    if (checksum) print_error("Header Checksum is invalid!");

    print_header(header);
    return header;
}


void load_rom(char filename[]) {
	/* Read a rom file and load contents */
    printf("Attempting to load rom file %s\n", filename);
	FILE *f;
	f = fopen(filename, "rb");
	if (f == NULL) {
		print_error("File is empty.");
	}
	
    read_rom_header(f);

	// unsigned char header[HEADER_SIZE];
	// fread(header, HEADER_SIZE, 1, f);
	// if (!check_header(header)) return 2;
	
	// int c;
	// while (!(max_graphs == 0) && (c = fgetc(f)) != EOF) {

	// 	order = (uint8_t)c;
		
	// 	uint8_t graph[order][order];
	// 	for (uint8_t i=0; i<order; i++) {
	// 		for (uint8_t j=0; j<order; j++) {
	// 			graph[i][j] = 0;
	// 		}
	// 		int go = 1;
	// 		do {
	// 			uint8_t c = fgetc(f);
	// 			if (c) {
	// 				graph[i][c-1] = 1;
	// 			} else {
	// 				go = 0;
	// 			}
	// 		} while (go);
	// 	}
		
	// 	print_graph((uint8_t *)graph);
		
	// 	uint64_t vertices = 1ULL;
	// 	int parents[order];
	// 	get_ham_cycles(*graph, 0, vertices, parents, 1);
	// 	if (max_graphs > 0) max_graphs--;
		
	// }
	fclose(f);
	// return 0;
}