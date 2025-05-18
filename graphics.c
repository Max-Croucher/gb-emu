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

clock_t start,end;

double frametime = 0;
uint8_t framecount_offset = 0;
#define FRAMETIME_BUFSIZE 10

uint8_t xoffset = 0;
uint32_t dot = 0;

GLubyte texture[144][160][3];
ObjectAttribute objects[10];

void init_screen_tex(GLubyte texture[144][160][3]) {
    /* initialise the screen texture */
    for (int i=0; i<144; i++) {
        float r = (float)((i+xoffset)%144) / 144.0 * 255.0;
        for (int j=0; j<160; j++) {
            float g = (float)j / 160 * 255.0;
            texture[i][j][0] = (GLubyte)r;
            texture[i][j][1] = (GLubyte)g;
            texture[i][j][2] = (GLubyte)0;
        }
    }
    for (int i=40; i < 60; i++) {
        for (int j=40; j < 60; j++) {
            texture[i][j][0] = (GLubyte)0;
            texture[i][j][1] = (GLubyte)0;
            texture[i][j][2] = (GLubyte)0;
            
        }
            
    }
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
    init_screen_tex(texture);
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
    start = clock();
    //glutMainLoop();
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


void load_tile(uint8_t* ram, uint16_t tile_data[8], uint8_t tile_id, bool is_object) {
    /* Read RAM to produce an 8x8 tile (stored in tile_data) */
    uint16_t tile_addr = 0x8000 + (uint16_t)(tile_id)*16;
    if (!(is_object) && tile_id < 128 && !(*(ram+0xFF40)&8)) tile_addr += 0x1000; // swap addressing mode if tile isn't an object
    for (int i=0; i<8; i++) {
        tile_data[i] = interleave(*(ram+tile_addr+(2*i)), *(ram+tile_addr+(2*i+1)));
    }
}


uint8_t read_objects(uint8_t* ram, ObjectAttribute attrbank[10], uint8_t scanline) {
    /* builds an array of up to 10 object attributes that intersect with the current scanline */
    uint8_t objects_found = 0;
    uint8_t offset_scanline = scanline + 16;
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


void tick_graphics(uint8_t *ram) {
    /* Main tick procedure for graphics */
    uint8_t scanline = dot/456;
    if (dot == 65564) { // enter VBLANK
        set_render_blocking_mode(1);
        *(ram+0xFF0F) |= 1; // Request a VBlank interrupt
        xoffset++;
        if (xoffset == 144) xoffset = 0;
        //glutMainLoopEvent();
        //glutPostRedisplay();
        //print_tilemaps(ram);
        end = clock();
        frametime += ((double)(end-start)) / CLOCKS_PER_SEC;
        framecount_offset++;
        if (framecount_offset == (FRAMETIME_BUFSIZE-1)) {
            framecount_offset = 0;
            fprintf(stderr, "framerate: %.2fHz\n", 1.0/(frametime/FRAMETIME_BUFSIZE));
            frametime = 0;
        }
        fprintf(stderr,"framerate: %.2fHz\n", 1.0/(frametime/10));
        start = clock();
    } else if ((scanline < 144) && (dot % 456) == 0) { // New scanline
        set_render_blocking_mode(2);
        ObjectAttribute objects[10];
        uint8_t objects_found = read_objects(ram, objects, scanline);

    } else if ((scanline < 144) && (dot % 456) == 80) { // Enter drawing mode
        set_render_blocking_mode(3);

    } else if ((scanline < 144) && (dot % 456) == 252) { // Enter Hblank
        set_render_blocking_mode(0);
    }





    dot++;
    if (dot == 70224) {
        dot = 0;
    }
}


void print_tilemaps(uint8_t *ram) {
    for (int i=0; i<32; i++) {
        uint16_t tiles[32][8];
        char print_palette[4] = {' ', '.', 'o', '0'};
        for (int j=0; j<32; j++) {
            load_tile(ram, tiles[j], *(ram+0x9800+(i*32)+j), 0);
        }
        for (int k=0; k<8; k++) {
            for (int j=0; j<32; j++) {
                for (int l=7; l>=0; l--){
                    printf("%c", print_palette[(tiles[j][k]>>(2*l))&3]);
                }
            }
            printf("\n");
        }
    }
}