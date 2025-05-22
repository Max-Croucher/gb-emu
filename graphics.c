/* Source file for graphics.c, controlling gameboy graphics
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <GL/freeglut.h>
#include <time.h>
#include "cpu.h"
#include "graphics.h"

extern uint8_t* ram;
extern JoypadState joypad_state;
extern bool LOOP;

clock_t start,end;

double frametime = 0;
uint8_t framecount_offset = 0;
#define FRAMETIME_BUFSIZE 10

uint8_t xoffset = 0;
static uint32_t dot = 0;

GLubyte texture[144][160][3];
ObjectAttribute objects[10];

uint8_t pixvals[5] = {0xF8, 0xA0, 0x50, 0x00, 0xFF};

bool old_stat_state = 0;

bool lcd_enable = 0;

void init_screen_tex(void) {
    glEnable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D,0,3,160,144,0,GL_RGB, GL_UNSIGNED_BYTE, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void gl_tick(void) {
    /* update graphics */
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    //glColor3f(0.0,1.0,0.0);
    init_screen_tex();
    glBegin(GL_POLYGON);
        glTexCoord2f(0,0); glVertex2f(0,0);
        glTexCoord2f(0,1); glVertex2f(0,1);
        glTexCoord2f(1,1); glVertex2f(1,1);
        glTexCoord2f(1,0); glVertex2f(1,0);
    glEnd();
	glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glutSwapBuffers();
}


void reshape_window(int w, int h) {
    /* Ensure window is accurately scaled*/
    glViewport(0,0,(GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,1,0,1,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void init_graphics(int *argc, char *argv[]) {
    /* Main init procedure for graphics */
    glutInit(argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(320,288);
    glutInitWindowPosition(100,100);
    glutCreateWindow("GB Emu");
    glClearColor(0.0,0.0,0.0,0.0);
    glShadeModel(GL_FLAT);
    glutDisplayFunc(gl_tick);
    glutReshapeFunc(reshape_window);
    blank_screen();
    glutKeyboardFunc(key_pressed);
    glutKeyboardUpFunc(key_released);
    glutCloseFunc(window_closed);
    glutMainLoopEvent();
    start = clock();
    *(ram+0xFF41) &= 0xFC; // set ppu mode to 0
    //glutMainLoop();
}


void window_closed(void) {
    /* execute when the window is closed */
    LOOP = 0;
}

void key_pressed(unsigned char key, int x, int y) {
    /* handle keys being pressed */
    bool has_changed = 1;
    switch (key)
    {
    case 'w':
        joypad_state.up = 1;
        break;
    case 's':
        joypad_state.down = 1;
        break;
    case 'a':
        joypad_state.left = 1;
        break;
    case 'd':
        joypad_state.right = 1;
        break;
    case ',':
        joypad_state.B = 1;
        break;
    case '.':
        joypad_state.A = 1;
        break;
    case ';':
        joypad_state.start = 1;
        break;
    case '\'':
        joypad_state.select = 1;
        break;
    case 'q':
    case 27:
        LOOP = 0;
        break;
    default:
        has_changed = 1;
    }
    if (has_changed) {
        if (key != 27) fprintf(stderr, "Key Pressed (%c)\n", key);
        joypad_io();
        bool old_state = *(ram+0xFF00)&0xF;
        if (old_state > *(ram+0xFF00)&0xF) *(ram+0xFF0F) |= 16; // Request a VBlank interrupt
    }
}


void key_released(unsigned char key, int x, int y) {
    /* handle keys being released */
    bool has_changed = 1;
    switch (key)
    {
    case 'w':
        joypad_state.up = 0;
        break;
    case 's':
        joypad_state.down = 0;
        break;
    case 'a':
        joypad_state.left = 0;
        break;
    case 'd':
        joypad_state.right = 0;
        break;
    case ',':
        joypad_state.B = 0;
        break;
    case '.':
        joypad_state.A = 0;
        break;
    case ';':
        joypad_state.start = 0;
        break;
    case '\'':
        joypad_state.select = 0;
        break;
    default:
        has_changed = 1;
    }
    if (has_changed) joypad_io();
}


void blank_screen(void) {
    /* set the entire screen to black */
    for (int i=0; i<144; i++) {
        for (int j=0; j<160; j++) {
            texture[i][j][0]=texture[i][j][1]=texture[i][j][2] = pixvals[4];
        }
    }
}

uint16_t interleave(uint16_t a, uint16_t b) {
    /* interleave the lower 8 bits in a and b, with the bits in b coming first
        eg: a = 0000000012345678
            b = 00000000abcdefgh
        becomes a1b2c3d4e5f6g7h8
    */
    uint16_t a1 = ((a &0x00F0)<<4) + (a &0x000F); // shift first half (000000012345678 -> 0000123400005678)
    uint16_t a2 = ((a1&0x0C0C)<<2) + (a1&0x0303); // shift first and third pair (0000123400005678 -> 0012003400560078)
    uint16_t a3 = ((a2&0x2222)<<1) + (a2&0x1111); // shift odd bits (0012003400560078 -> 0102030405060708)
    
    uint16_t b1 = ((b &0x00F0)<<4) + (b &0x000F); // as above, with b
    uint16_t b2 = ((b1&0x0C0C)<<2) + (b1&0x0303);
    uint16_t b3 = ((b2&0x2222)<<2) + ((b2&0x1111)<<1); // one bit further over

    return a3 + b3;
}

uint8_t get_tile_id(uint16_t tilepos, bool is_window_layer) {
    uint16_t base_addr;
    if ((is_window_layer && (*(ram+0xFF40)&64)) || (!is_window_layer && (*(ram+0xFF40)&8))) {
        base_addr = 0x9C00;
    } else {
        base_addr = 0x9800;
    }
    return *(ram + base_addr + tilepos);
}

uint16_t get_tile_addr(uint8_t tile_id, bool is_object) {
    /* get the memory address of a tile from the tile id and the addressing mode register */
    uint16_t tile_addr = 0x8000 + ((uint16_t)(tile_id))*16;
    if (!(is_object) && tile_id < 128 && !(*(ram+0xFF40)&16)) tile_addr += 0x1000; // swap addressing mode if tile isn't an object
    return tile_addr;
}


void load_tile(uint16_t tile_data[8], uint16_t tile_addr) {
    /* Read RAM to produce an 8x8 tile (stored in tile_data) */
    
    for (int i=0; i<8; i++) {
        tile_data[i] = interleave(*(ram+tile_addr+(2*i)), *(ram+tile_addr+(2*i)+1));
    }
}


uint8_t read_objects(ObjectAttribute attrbank[10]) {
    /* builds an array of up to 10 object attributes that intersect with the current scanline */
    uint8_t objects_found = 0;
    uint8_t offset_scanline = *(ram+0xFF44) + 16;
    bool tile8x8 = *(ram+0xFF40) & (1<<2); //1 if 8x8, 0 if 8x16
    for (int i=0; i<40; i++) {
        if (objects_found == 10) break;
        uint8_t ypos = *(ram+0xFFE0+(i*4));
        if (
            (tile8x8 && ypos+8>offset_scanline) && //8x8 sprite isn't above scanline
            (!tile8x8 && ypos+16>offset_scanline) && //8x16 isn't above scanline
            (ypos<=offset_scanline) //sprite isn't below scanline
        ) { //Object intersects scanline
            uint8_t flags = *(ram+0xFFE0+(i*4)+3);
            attrbank[objects_found] = (ObjectAttribute){
                .ypos = ypos,
                .xpos = *(ram+0xFFE0+(i*4)+1),
                .tileid = *(ram+0xFFE0+(i*4)+2),
                .priority = flags & (1<<7),
                .yflip = flags & (1<<7),
                .xflip = flags & (1<<7),
                .palette = flags & (1<<7)
            };
            objects_found++;
        }
    }
    return objects_found;
}

uint8_t get_background_pallette(uint8_t pallette_id) {
    /* poll the BGP register */
    return (*(ram+0xFF47) >> (2*pallette_id))&3;
}

void draw_background() {
    /* Draw the background layer on the current scanline */
    if (*(ram+0xFF40)&1 == 0) { //bg is disabled
        for (int i=0; i<160; i++) {
            texture[143-*(ram+0xFF44)][i][0] = pixvals[0];
            texture[143-*(ram+0xFF44)][i][1] = pixvals[0];
            texture[143-*(ram+0xFF44)][i][2] = pixvals[0];
        }
        return;
    }
    uint8_t top = *(ram+0xFF44) + *(ram+0xFF42); //get scroll vals
    uint8_t left = *(ram+0xFF43);
    uint16_t tiles[21][8];
    for (int i=0; i<21; i++) {
        uint8_t tile_id = get_tile_id((left>>3)+i + (top>>3)*32, 0);
        load_tile(tiles[i], get_tile_addr(tile_id, 0));
    }
    for (int i=0; i<160; i++) {
        uint8_t pix = (tiles[(i+left)>>3][top%8] >> (14-(2*(((i+left)%8)))))&3;
        texture[143-*(ram+0xFF44)][i][0] = pixvals[get_background_pallette(pix)];
        texture[143-*(ram+0xFF44)][i][1] = pixvals[get_background_pallette(pix)];
        texture[143-*(ram+0xFF44)][i][2] = pixvals[get_background_pallette(pix)];
    }
}


void draw_window() {
    /* Draw the window layer on the current scanline */
}


void draw_objects(ObjectAttribute objects[10], uint8_t objects_found) {
    /* Draw the object layer on the current scanline */
}


bool tick_graphics(void) {
    /* Main tick procedure for graphics */

    if (lcd_enable ^ (*(ram+0xFF40)>>7)) { //lcd state change
        if (*(ram+0xFF40)>>7) { // LCD was just turned on
            lcd_enable = 1;
        } else { // LCD was just turned off
            dot = 0;
            lcd_enable = 0;
            *(ram+0xFF41) &= 0xFC;
            *(ram+0xFF41) += 0; // set ppu mode to 0
            blank_screen();
        }
    }
    *(ram+0xFF44) = (dot/456); // LY (scanline)
    *(ram+0xFF41) &= 0xFB;
    *(ram+0xFF41) |= (*(ram+0xFF44) == *(ram+0xFF45))<<2; // set LY=LYC flag

    bool stat_type_LYC = *(ram+0xFF41)&64;
    bool current_stat_state = (
        stat_type_LYC ||
        (((*(ram+0xFF41)>>5)&1) && ((*(ram+0xFF41)&3) == 2)) || // mode 2 is set & ppu is in mode 2
        (((*(ram+0xFF41)>>4)&1) && ((*(ram+0xFF41)&3) == 1)) || // mode 1 is set & ppu is in mode 1
        (((*(ram+0xFF41)>>3)&1) && ((*(ram+0xFF41)&3) == 0)) // mode 0 is set & ppu is in mode 0
    );
    if ((old_stat_state == 0) && current_stat_state) *(ram+0xFF0F) |= 2; // Request a STAT interrupt
    old_stat_state == current_stat_state;

    if (lcd_enable) {
        ObjectAttribute objects[10];
        uint8_t objects_found = 0;
        if (dot == 65564) { // enter VBLANK
            *(ram+0xFF41) &= 0xFC;
            *(ram+0xFF41) += 1; // set ppu mode to 1
            *(ram+0xFF0F) |= 1; // Request a VBlank interrupt
            xoffset++;
            if (xoffset == 144) xoffset = 0;
            glutMainLoopEvent();
            glutPostRedisplay();
            //print_tilemaps();
            end = clock();
            frametime += ((double)(end-start)) / CLOCKS_PER_SEC;
            framecount_offset++;
            if (framecount_offset == (FRAMETIME_BUFSIZE-1)) {
                framecount_offset = 0;
                fprintf(stderr, "framerate: %.2fHz\n", 1.0/(frametime/FRAMETIME_BUFSIZE));
                frametime = 0;
            }
            //fprintf(stderr,"framerate: %.2fHz\n", 1.0/(frametime/10));
            start = clock();
        } else if ((*(ram+0xFF44) < 144) && (dot % 456) == 0) { // New scanline
            *(ram+0xFF41) &= 0xFC;
            *(ram+0xFF41) += 2; // set ppu mode to 2
            objects_found = read_objects(objects);

        } else if ((*(ram+0xFF44) < 144) && (dot % 456) == 80) { // Enter drawing mode
            *(ram+0xFF41) &= 0xFC;
            *(ram+0xFF41) += 3; // set ppu mode to 3
            draw_background();
            draw_window();
            draw_objects(objects, objects_found);

        } else if ((*(ram+0xFF44) < 144) && (dot % 456) == 232) { // Enter Hblank
            *(ram+0xFF41) &= 0xFC; // set ppu mode to 0
        }

        dot++;
        if (dot == 70224) {
            dot = 0;
        }
        return !dot;
    } else {
        return 0;
    }
}


void print_tilemaps(void) {
    printf("FRAME\n");
    char print_palette[4] = {' ', '.', 'o', '0'};
    
    for (int i=0; i<32; i++) {
        uint16_t tiles[16][8];
        for (int j=0; j<16; j++) {
            //load_tile(tiles[j], get_tile_addr((i*16)+j, 0));
            load_tile(tiles[j], 0x8000 + ((uint16_t)((i*16)+j))*16);
        }
        for (int k=0; k<8; k++) {
            for (int j=0; j<16; j++) {
                for (int l=7; l>=0; l--){
                    printf("%c", print_palette[(tiles[j][k]>>(2*l))&3]);
                }
            }
            printf("\n");
        }
    }

    // for (int i=0; i<32; i++) {
    //     uint16_t tiles[32][8];
    //     for (int j=0; j<32; j++) {
    //         printf("%d, %d: %d | %.4x\n", i, j, get_tile_id(j+i*32, 0), get_tile_addr(get_tile_id(j+i*32, 0),0));
    //         load_tile(tiles[j], get_tile_addr(get_tile_id(j+i*32, 0),0));
    //     }
    //     for (int k=0; k<8; k++) {
    //         for (int j=0; j<32; j++) {
    //             for (int l=7; l>=0; l--){
    //                 printf("%c", print_palette[get_background_pallette((tiles[j][k]>>(2*l))&3)]);
    //             }
    //         }
    //         printf("\n");
    //     }
    // }


    // for (int i=0; i<144; i++) {
    //     for (int j=0; j<160; j++) {
    //         printf("%.2x", texture[i][j][0]);
    //     }
    //     printf("\n");
    // }
}