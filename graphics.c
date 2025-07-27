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
#include <png.h>
#include "cpu.h"
#include "rom.h"
#include "graphics.h"

extern uint8_t* ram;
extern uint8_t* rom;
extern JoypadState joypad_state;
extern bool LOOP;
extern bool hyperspeed;
extern bool debug_tilemap;
extern bool debug_scanlines;
extern bool dmg_colours;
extern bool frame_by_frame;
extern char* save_filename;
extern bool do_save_game;


clock_t start,end;
double rolling_frametime = 0;
uint8_t framecount_offset = 0;
uint8_t frame_report_offset = 0;

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define FRAMETIME_BUFSIZE 10
#define TARGET_FRAMETIME 0.016742706
#define FRAMETIME_REPORT_INTERVAL 12
double calibrated_frametime = TARGET_FRAMETIME;

char rom_name[16];
char window_name[32];
uint8_t xoffset = 0;
static uint32_t dot = 0;
uint8_t window_internal_counter = 0;
GLint WindowMain = 1;
GLint WindowDebug = 2;
GLubyte texture[SCREEN_HEIGHT][SCREEN_WIDTH][3];
GLubyte bg_tilemap[256][256][3];
GLubyte window_tilemap[256][256][3];
GLubyte object_tilemap[16][320][3];
GLubyte vram_block_1[32][256][3];
GLubyte vram_block_2[32][256][3];
GLubyte vram_block_3[32][256][3];
bool bgw_priority_map[SCREEN_HEIGHT][SCREEN_WIDTH];
ObjectAttribute objects[10];
uint8_t objects_found = 0;
uint8_t pixvals[5][3] = {{0xF8,0xF8,0xF8}, {0xA0,0xA0,0xA0}, {0x50,0x50,0x50}, {0x00,0x00,0x00}, {0xFF,0xFF,0xFF}};
uint8_t dmgcols[5][3] = {{155,188,15},{139,172,15},{48,98,48},{15,56,15},{155*1.2,188*1.2,15*1.2}};
uint8_t priority_object[SCREEN_WIDTH];
bool old_stat_state = 0;
bool lcd_enable = 0;
bool debug_used_objects[40] = {0};
int debug_frameskip = 1;
int debug_frames_done = 0;


static inline void framerate(void) {
    /* run at the start of VBLANK to compute framerate and add delay to target 59.73Hz */
    double raw_frametime = ((double)(clock()-start)) / CLOCKS_PER_SEC;
    if (!hyperspeed) {
        double catchup = calibrated_frametime - raw_frametime;
        struct timespec waittime;
        waittime.tv_sec = 0;
        waittime.tv_nsec = (long int)(1000000000 * catchup);
        nanosleep(&waittime, &waittime);
        raw_frametime = catchup + ((double)(clock()-start)) / CLOCKS_PER_SEC;
    }

    rolling_frametime += raw_frametime;
    framecount_offset++;
    if (framecount_offset == FRAMETIME_BUFSIZE) {
        framecount_offset = 0;
        if (frame_report_offset == FRAMETIME_REPORT_INTERVAL) {
            frame_report_offset = 0;
            sprintf(window_name, "%s | %.1fHz", rom_name, 1.0/(rolling_frametime/FRAMETIME_BUFSIZE));
            glutSetWindowTitle(window_name);
        }
        frame_report_offset++;
        if (!hyperspeed) {
            double frametime_error = TARGET_FRAMETIME - rolling_frametime/FRAMETIME_BUFSIZE;
            calibrated_frametime += frametime_error/2;
        }
        rolling_frametime = 0;
    }
    start = clock();
}


void gl_tick(void) {
    /* update graphics */
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D,0,3,SCREEN_WIDTH,SCREEN_HEIGHT,0,GL_RGB, GL_UNSIGNED_BYTE, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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


static inline void object_map_draw_quad(uint8_t i) {
    /* draw an object's sprite on the tilemap debugger */
    float height_unit = 72.0/512;
    glBegin(GL_POLYGON);
        glTexCoord2f(((float)i)/40,0); glVertex2f((5 + 38.0*i)/1728,height_unit*1.75);
        glTexCoord2f(((float)i)/40,1); glVertex2f((5 + 38.0*i)/1728,height_unit*2.75);
        glTexCoord2f(((float)i+1)/40,1); glVertex2f((5 + 38.0*i + 36)/1728,height_unit*2.75);
        glTexCoord2f(((float)i+1)/40,0); glVertex2f((5 + 38.0*i + 36)/1728,height_unit*1.75);
    glEnd();
    glBegin(GL_POLYGON);
        glTexCoord2f(((float)i)/40+0.5,0); glVertex2f((5 + 38.0*i)/1728,height_unit*0.25);
        glTexCoord2f(((float)i)/40+0.5,1); glVertex2f((5 + 38.0*i)/1728,height_unit*1.25);
        glTexCoord2f(((float)i+1)/40+0.5,1); glVertex2f((5 + 38.0*i + 36)/1728,height_unit*1.25);
        glTexCoord2f(((float)i+1)/40+0.5,0); glVertex2f((5 + 38.0*i + 36)/1728,height_unit*0.25);
    glEnd();
}


static void draw_text(float x, float y, void *font, const char* string) {
    /* draw a string on the screen */  
  glColor3f(0, 0, 0); 
  glRasterPos2f(x, y);
  glutBitmapString(font, (const unsigned char*)string);
}


static inline void highlight_object(uint8_t i) {
    /* highlight a used object on the tilemap debugger */
    char string[32];
    sprintf(string, "%.2X,%.2X", *(ram + 0xFE01 + i*4), *(ram + 0xFE00 + i*4));
    draw_text((5 + 38.0*(i%20))/1728,67.0/512*(i<20 ? 1.75 : 0.14), GLUT_BITMAP_HELVETICA_10, string);

    float left = (5 + 38.0*(i%20))/1728;
    float right = (5 + 38.0*(i%20)+36)/1728;
    float bottom = 72.0/512*(i<20 ? 2.75 : 1.25);
    float top;
    if ((*(ram+REG_LCDC)>>2)&1) {
        top = 72.0/512*(i<20 ? 1.75 : 0.25);
    } else {
        top = 72.0/512*(i<20 ? 2.25 : 0.75);
    }

    glColor3f(1, 0, 0); 
    glBegin(GL_LINE_LOOP);
        glVertex2f(left,  top); // XYZ left, top
        glVertex2f(right, top); // XYZ right, top
        glVertex2f(right, bottom); // XYZ right, bottom
        glVertex2f(left,  bottom); // XYZ left, bottom
    glEnd();

}

void gl_tick_debug_window(void) {
    /* update debug window */
    glClear(GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D,0,3,256,256,0,GL_RGB, GL_UNSIGNED_BYTE, bg_tilemap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBegin(GL_POLYGON);
        glTexCoord2f(0,0); glVertex2f(0.25/27,0.475);
        glTexCoord2f(0,1); glVertex2f(0.25/27,0.975);
        glTexCoord2f(1,1); glVertex2f(4.25/27,0.975);
        glTexCoord2f(1,0); glVertex2f(4.25/27,0.475);
    glEnd();
    glTexImage2D(GL_TEXTURE_2D,0,3,256,256,0,GL_RGB, GL_UNSIGNED_BYTE, window_tilemap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBegin(GL_POLYGON);
        glTexCoord2f(0,0); glVertex2f(7.75/27,0.475);
        glTexCoord2f(0,1); glVertex2f(7.75/27,0.975);
        glTexCoord2f(1,1); glVertex2f(11.75/27,0.975);
        glTexCoord2f(1,0); glVertex2f(11.75/27,0.475);
    glEnd();
    glTexImage2D(GL_TEXTURE_2D,0,3,256,32,0,GL_RGB, GL_UNSIGNED_BYTE, vram_block_3);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBegin(GL_POLYGON);
        glTexCoord2f(0,0); glVertex2f(39.0/72,04.0/64);
        glTexCoord2f(0,1); glVertex2f(39.0/72,16.0/64);
        glTexCoord2f(1,1); glVertex2f(71.0/72,16.0/64);
        glTexCoord2f(1,0); glVertex2f(71.0/72,04.0/64);
    glEnd();
    glTexImage2D(GL_TEXTURE_2D,0,3,256,32,0,GL_RGB, GL_UNSIGNED_BYTE, vram_block_2);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBegin(GL_POLYGON);
        glTexCoord2f(0,0); glVertex2f(39.0/72,24.0/64);
        glTexCoord2f(0,1); glVertex2f(39.0/72,36.0/64);
        glTexCoord2f(1,1); glVertex2f(71.0/72,36.0/64);
        glTexCoord2f(1,0); glVertex2f(71.0/72,24.0/64);
    glEnd();
    glTexImage2D(GL_TEXTURE_2D,0,3,256,32,0,GL_RGB, GL_UNSIGNED_BYTE, vram_block_1);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBegin(GL_POLYGON);
        glTexCoord2f(0,0); glVertex2f(39.0/72,44.0/64);
        glTexCoord2f(0,1); glVertex2f(39.0/72,56.0/64);
        glTexCoord2f(1,1); glVertex2f(71.0/72,56.0/64);
        glTexCoord2f(1,0); glVertex2f(71.0/72,44.0/64);
    glEnd();
    glTexImage2D(GL_TEXTURE_2D,0,3,320,16,0,GL_RGB, GL_UNSIGNED_BYTE, object_tilemap);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    for (int i=0; i<20; i++) {
        object_map_draw_quad(i);
    }
	glDisable(GL_TEXTURE_2D);
    char string[64];
    sprintf(string, "LCD Enable:    %s", ((*(ram+REG_LCDC)>>7)&1) ? "ON" : "OFF");
    draw_text(0.16,0.96, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "Window Map:    %s", ((*(ram+REG_LCDC)>>6)&1) ? "9C00-9FFF" : "9800-9BFF");
    draw_text(0.16,0.93, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "Window Enable: %s", ((*(ram+REG_LCDC)>>5)&1) ? "ON" : "OFF");
    draw_text(0.16,0.90, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "W/BG Tiledata: %s", ((*(ram+REG_LCDC)>>4)&1) ? "8000-8FFF" : "8800-97FF");
    draw_text(0.16,0.87, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "Backround Map: %s", ((*(ram+REG_LCDC)>>3)&1) ? "9C00-9FFF" : "9800-9BFF");
    draw_text(0.16,0.84, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "OBJ Size:      %s", ((*(ram+REG_LCDC)>>2)&1) ? "8x16" : "8x8");
    draw_text(0.16,0.81, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "OBJ Enable:    %s", ((*(ram+REG_LCDC)>>1)&1) ? "ON" : "OFF");
    draw_text(0.16,0.78, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "W/BG Enable:   %s", ((*(ram+REG_LCDC)>>0)&1) ? "ON" : "OFF");
    draw_text(0.16,0.75, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "LY | LYC  = 0x%.2X | 0x%.2X", *(ram+REG_LY), *(ram+REG_LYC));
    draw_text(0.16,0.72, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "STAT      = 0b %d%d%d%d %d%d%d%d", (*(ram+REG_STAT)>>7)&1,(*(ram+REG_STAT)>>6)&1,(*(ram+REG_STAT)>>5)&1,(*(ram+REG_STAT)>>4)&1,(*(ram+REG_STAT)>>3)&1,(*(ram+REG_STAT)>>2)&1,(*(ram+REG_STAT)>>1)&1,(*(ram+REG_STAT)>>0)&1);
    draw_text(0.16,0.69, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "SCX | SCY = 0x%.2X | 0x%.2X", *(ram+REG_SCX), *(ram+REG_SCY));
    draw_text(0.16,0.66, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "WX+7 | WY = 0x%.2X | 0x%.2X", *(ram+REG_WY), *(ram+REG_WX));
    draw_text(0.16,0.63, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "W Count   = 0x%.2X", window_internal_counter);
    draw_text(0.16,0.60, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "BGP       = 0x%.2X", *(ram+REG_BGP));
    draw_text(0.16,0.57, GLUT_BITMAP_9_BY_15, string);
    sprintf(string, "OBP0|OBP1 = 0x%.2X | 0x%.2X", *(ram+REG_OBP0), *(ram+REG_OBP1));
    draw_text(0.16,0.54, GLUT_BITMAP_9_BY_15, string);

    draw_text(0.00222,0.4, GLUT_BITMAP_9_BY_15, "0x00");
    draw_text(1.6/9,0.4, GLUT_BITMAP_9_BY_15, "0x08");
    draw_text(3.2/9,0.4, GLUT_BITMAP_9_BY_15, "0x10");
    draw_text(0.09111,0.19, GLUT_BITMAP_9_BY_15, "0x18");
    draw_text(0.26888,0.19, GLUT_BITMAP_9_BY_15, "0x20");

    draw_text(0.505,54.0/64, GLUT_BITMAP_9_BY_15, "0x8000");
    draw_text(0.505,51.0/64, GLUT_BITMAP_9_BY_15, "0x8200");
    draw_text(0.505,48.0/64, GLUT_BITMAP_9_BY_15, "0x8400");
    draw_text(0.505,45.0/64, GLUT_BITMAP_9_BY_15, "0x8600");
    draw_text(0.505,34.0/64, GLUT_BITMAP_9_BY_15, "0x8800");
    draw_text(0.505,31.0/64, GLUT_BITMAP_9_BY_15, "0x8A00");
    draw_text(0.505,28.0/64, GLUT_BITMAP_9_BY_15, "0x8C00");
    draw_text(0.505,25.0/64, GLUT_BITMAP_9_BY_15, "0x8E00");
    draw_text(0.505,14.0/64, GLUT_BITMAP_9_BY_15, "0x9000");
    draw_text(0.505,11.0/64, GLUT_BITMAP_9_BY_15, "0x9200");
    draw_text(0.505,08.0/64, GLUT_BITMAP_9_BY_15, "0x9400");
    draw_text(0.505,05.0/64, GLUT_BITMAP_9_BY_15, "0x9600");
    for (float i=33; i<140; i+=40) {
        draw_text(39.0/72,i/128, GLUT_BITMAP_9_BY_15, "000");
        draw_text(41.0/72,i/128, GLUT_BITMAP_9_BY_15, "020");
        draw_text(43.0/72,i/128, GLUT_BITMAP_9_BY_15, "040");
        draw_text(45.0/72,i/128, GLUT_BITMAP_9_BY_15, "060");
        draw_text(47.0/72,i/128, GLUT_BITMAP_9_BY_15, "080");
        draw_text(49.0/72,i/128, GLUT_BITMAP_9_BY_15, "0A0");
        draw_text(51.0/72,i/128, GLUT_BITMAP_9_BY_15, "0C0");
        draw_text(53.0/72,i/128, GLUT_BITMAP_9_BY_15, "0E0");
        draw_text(55.0/72,i/128, GLUT_BITMAP_9_BY_15, "100");
        draw_text(57.0/72,i/128, GLUT_BITMAP_9_BY_15, "120");
        draw_text(59.0/72,i/128, GLUT_BITMAP_9_BY_15, "140");
        draw_text(61.0/72,i/128, GLUT_BITMAP_9_BY_15, "160");
        draw_text(63.0/72,i/128, GLUT_BITMAP_9_BY_15, "180");
        draw_text(65.0/72,i/128, GLUT_BITMAP_9_BY_15, "1A0");
        draw_text(67.0/72,i/128, GLUT_BITMAP_9_BY_15, "1C0");
        draw_text(69.0/72,i/128, GLUT_BITMAP_9_BY_15, "1E0");
    }
    for (float i=0.5; i<50; i+=46.7) {
        for (int j=0; j<16; j++) {
            float ypos = 122.8 - 4*j;
            char c[2] = {(j%8)*2 + (j%8 > 4 ? 55 : 48), 0};
            draw_text(i/108, ypos/128, GLUT_BITMAP_HELVETICA_10, c);
        }
    }
    for (float i=1.1; i<50; i+= 30) {
        for (int j=0; j<16; j++) {
            float xpos = i + j;
            char c[2] = {(j%8)*2 + (j%8 > 4 ? 55 : 48), 0};
            draw_text(xpos/108, 125.0/128, GLUT_BITMAP_HELVETICA_10, c);
        }
    }

    for (int i=0; i<40; i++) {
        if (debug_used_objects[i]) {
            highlight_object(i);
            debug_used_objects[i] = 0;
        }
    }

    glColor3f(1,1,1);
    glutSwapBuffers();
}


void reshape_window_ratio(int w, int h) {
    /* Ensure window is accurately scaled */
    if (w * SCREEN_HEIGHT > h * SCREEN_WIDTH) { // window is too wide
        int target_width = (h * SCREEN_WIDTH) / SCREEN_HEIGHT;
        int border = (w - target_width) / 2;
        glViewport(border,0,(GLsizei)(target_width), (GLsizei)h);
    } else { // window is too tall
        int target_height = (w * SCREEN_HEIGHT) / SCREEN_WIDTH;
        int border = (h - target_height) / 2;
        glViewport(0,border,(GLsizei)w, (GLsizei)(target_height));
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,1,0,1,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void reshape_window_free(int w, int h) {
    /* Ensure window is accurately scaled */
    glViewport(0,0,(GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,1,0,1,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


static void blank_screen(void) {
    /* set the entire screen to black */
    for (int i=0; i<SCREEN_HEIGHT; i++) {
        for (int j=0; j<SCREEN_WIDTH; j++) {
            texture[i][j][0]=pixvals[4][0];
            texture[i][j][1]=pixvals[4][1];
            texture[i][j][2]=pixvals[4][2];
        }
    }
}


void init_graphics(int *argc, char *argv[], char rom_title[16]) {
    /* Main init procedure for graphics */

    //shared init
    glutInit(argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);

    //init main window
    glutInitWindowSize(SCREEN_WIDTH*3,SCREEN_HEIGHT*3);
    glutInitWindowPosition(100,20);
    memcpy(rom_name, rom_title, 16);
    WindowMain = glutCreateWindow(rom_name);
    glClearColor(0.0,0.0,0.0,0.0);
    glShadeModel(GL_FLAT);
    glutDisplayFunc(gl_tick);
    glutReshapeFunc(reshape_window_ratio);
    blank_screen();
    glutKeyboardFunc(key_pressed);
    glutKeyboardUpFunc(key_released);
    glutCloseFunc(window_closed);

    
    //init debug menu
    if (debug_tilemap) {
        glutInitWindowSize(1729, 513);
        glutInitWindowPosition(0,500);
        WindowDebug = glutCreateWindow("Tilemaps and Objects");
        glClearColor(0.9,0.9,0.9,0.0);
        glShadeModel(GL_FLAT);
        glutDisplayFunc(gl_tick_debug_window);
        glutReshapeFunc(reshape_window_free);
        glutKeyboardFunc(key_pressed);
        glutKeyboardUpFunc(key_released);
        glutCloseFunc(window_closed);
    }

    if (dmg_colours) memcpy(pixvals, dmgcols, 15);

    //start
    glutMainLoopEvent();
    start = clock();
    *(ram+REG_STAT) &= 0xFC; // set ppu mode to 0
}


void window_closed(void) {
    /* execute when the window is closed */
    LOOP = 0;
}


void take_screenshot(char *filename) {
    /* take a screenshot by writing the global array 'texture' to a png */    
    FILE *png_file = fopen(filename, "wb");
    if (!png_file) { // opening file failed
        return;
    }
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) { // creating write struct failed
        fclose(png_file);
        return;
    }
    png_infop png_info = png_create_info_struct(png_ptr);
    if (png_info == NULL || setjmp(png_jmpbuf(png_ptr))) { // creating info struct failed
        png_destroy_write_struct(&png_ptr, &png_info);
        fclose(png_file);
        return;
    }
    
    png_set_IHDR(
        png_ptr,
        png_info,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        8, // bit depth per channel
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_byte **row_pointers = png_malloc(png_ptr, SCREEN_HEIGHT * sizeof(png_byte *));
    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
        row_pointers[y] = png_malloc(png_ptr, sizeof(uint8_t) * SCREEN_WIDTH * 3);
        memcpy(row_pointers[y], texture[SCREEN_HEIGHT-y-1], SCREEN_WIDTH * sizeof(uint8_t) * 3);
    }
    
    png_init_io(png_ptr, png_file);
    png_set_rows(png_ptr, png_info, row_pointers);
    png_write_png(png_ptr, png_info, PNG_TRANSFORM_IDENTITY, NULL);
    
    // free arrays and clean up
    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
        png_free(png_ptr, row_pointers[y]);
    }
    png_free(png_ptr, row_pointers);
}


void key_pressed(unsigned char key, int x, int y) {
    /* handle keys being pressed */
    bool has_changed = 0;
    uint16_t keyboard_modifiers = glutGetModifiers();
    if (keyboard_modifiers == GLUT_ACTIVE_CTRL) { // handle ctrl + <key>
        switch (key + 96) // if ctrl is pressed then it masks out 0x60
        {
        case 's': // ctrl + s (save external RAM)
            if (do_save_game) save_external_ram(save_filename);
            break;
        case 'f':; // ctrl + f (take screenshot)
            time_t curr_time = time(NULL);
            struct tm time_struct; memset(&time_struct, 0, sizeof(time_struct));
            gmtime_r(&curr_time, &time_struct);
            char timestamp_filename[64];
            sprintf(timestamp_filename, "screenshot_%04d-%02d-%02d_%02d.%02d.%02d.png\n",
                1900+time_struct.tm_year, time_struct.tm_mon+1, time_struct.tm_mday, time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);
            take_screenshot(timestamp_filename);
            break;
        }

    } else if (keyboard_modifiers) {
        // skip other modifiers (alt, shift, super)
    } else { // only register key press if shift, ctrl, etc. keys are not held
        has_changed = 1;
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
            has_changed = 0;
        }
    }
    if (has_changed) joypad_io();
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
        has_changed = 0;
    }
    if (has_changed) joypad_io();
}


static uint16_t interleave(uint16_t a, uint16_t b) {
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


static uint8_t get_tile_id(uint16_t tilepos, bool is_window_layer) {
    uint16_t base_addr;
    if ((is_window_layer && (*(ram+REG_LCDC)&64)) || ((!is_window_layer) && (*(ram+REG_LCDC)&8))) {
        base_addr = 0x9C00;
    } else {
        base_addr = 0x9800;
    }
    return *(ram + base_addr + tilepos);
}

static uint16_t get_tile_addr(uint8_t tile_id, bool is_object) {
    /* get the memory address of a tile from the tile id and the addressing mode register */
    if (is_object || (*(ram+REG_LCDC)&16)) { // UNSIGNED addressing from 0x8000
        return 0x8000 + tile_id*16;
    } else {// SIGNED addressing from 0x9000
        tile_id ^= 128;
        return 0x8800 + tile_id*16;
        //return 0x9000 + (int8_t)(tile_id) * 16;
    }
}


static void load_tile(uint16_t tile_data[8], uint16_t tile_addr) {
    /* Read RAM to produce an 8x8 tile (stored in tile_data) */
    
    for (int i=0; i<8; i++) {
        tile_data[i] = interleave(*(ram+tile_addr+(2*i)), *(ram+tile_addr+(2*i)+1));
    }
}


static inline void read_objects(void) {
    /* builds an array of up to 10 object attributes that intersect with the current scanline */
    objects_found = 0;
    uint8_t offset_scanline = *(ram+REG_LY) + 16;
    bool tile8x16 = *(ram+REG_LCDC) & 4; //1 if 8x8, 0 if 8x16
    for (int i=0; i<40; i++) {
        if (objects_found == 10) break;
        uint8_t ypos = *(ram+0xFE00+(i*4));
        if (
            (!(tile8x16) && ypos<=offset_scanline && ypos+8>offset_scanline) || 
            (tile8x16 && ypos<=offset_scanline  && ypos+16>offset_scanline)
        ) { //Object intersects scanline
            uint8_t flags = *(ram+0xFE00+(i*4)+3);
            objects[objects_found] = (ObjectAttribute){
                .ypos = ypos,
                .xpos = *(ram+0xFE00+(i*4)+1),
                .tileid = *(ram+0xFE00+(i*4)+2),
                .priority = (bool)(flags & (1<<7)),
                .yflip = (bool)(flags & (1<<6)),
                .xflip = (bool)(flags & (1<<5)),
                .palette = (bool)(flags & (1<<4))
            };
            debug_used_objects[i] = 1;
            objects_found++;
        }
    }
}


static uint8_t get_background_palette(uint8_t palette_value) {
    /* poll the BGP register */
    return (*(ram+REG_BGP) >> (2*palette_value))&3;
}


static uint8_t get_object_palette(bool palette_id, uint8_t palette_value) {
    /* poll the BGP register */
    if (palette_id) {
        return (*(ram+REG_OBP1) >> (2*palette_value))&3;
    }
    return (*(ram+REG_OBP0) >> (2*palette_value))&3;
}


static inline int compare_obj_xvalue(const void *a, const void *b) {
    /* compare to ObjectAttributes by xpos, for sorting */
    ObjectAttribute *objA = (ObjectAttribute *)a;
    ObjectAttribute *objB = (ObjectAttribute *)b;
  
    return ((int)objA->xpos) - ((int)objB->xpos);
}


static inline void draw_background() {
    /* Draw the background layer on the current scanline */
    if ((*(ram+REG_LCDC)&1) == 0) { //bg is disabled
        for (int i=0; i<SCREEN_WIDTH; i++) {
            texture[143-*(ram+REG_LY)][i][0] = pixvals[0][0];
            texture[143-*(ram+REG_LY)][i][1] = pixvals[0][1];
            texture[143-*(ram+REG_LY)][i][2] = pixvals[0][2];
        }
        return;
    }
    uint8_t top = *(ram+REG_LY) + *(ram+REG_SCY); //get scroll vals
    uint8_t left = *(ram+REG_SCX);
    uint16_t tiles[21][8];
    for (uint8_t i=0; i<21; i++) {
        uint8_t tile_id = get_tile_id((((left>>3)+i)%32) + (top>>3)*32, 0);
        load_tile(tiles[i], get_tile_addr(tile_id, 0));
    }

    for (uint8_t i=0; i<SCREEN_WIDTH; i++) {
        uint8_t pix = (tiles[((i+left%8))>>3][top%8] >> (14-(2*(((i+left)%8)))))&3;
        bgw_priority_map[143-*(ram+REG_LY)][i] = (bool)pix;
        texture[143-*(ram+REG_LY)][i][0] = pixvals[get_background_palette(pix)][0];
        texture[143-*(ram+REG_LY)][i][1] = pixvals[get_background_palette(pix)][1];
        texture[143-*(ram+REG_LY)][i][2] = pixvals[get_background_palette(pix)][2];
    }
}


static inline void draw_window() {
    /* Draw the window layer on the current scanline */
    if ((*(ram+REG_LCDC)&0x21) == 0x21) { // Window Enable and BG/Window Enable bits are set
        if (*(ram+REG_LY) >= *(ram+REG_WX)) { // when scanline >= window Y
            bool pixel_drawn = 0;
            uint16_t tiles[21][8];
            for (int i=0; i<21; i++) {
                uint8_t tile_id = get_tile_id(i + (window_internal_counter>>3)*32, 1);
                load_tile(tiles[i], get_tile_addr(tile_id, 0));
            }
            for (int screen_x_pos = *(ram+REG_WY) - 7; screen_x_pos < SCREEN_WIDTH; screen_x_pos++) {
                pixel_drawn = 1;
                uint8_t window_x_pos = screen_x_pos - (*(ram+REG_WY) - 7);
                //screen_y_pos is *(ram+REG_LY)
                //window_y_pos is window_internal_counter


                //draw pixel at (window_x_pos, window_y_pos) to (screen_x_pos, screen_y_pos)
                uint8_t pix = (tiles[window_x_pos>>3][window_internal_counter%8] >> (14-(2*((window_x_pos%8)))))&3;
                bgw_priority_map[143-*(ram+REG_LY)][screen_x_pos] |= (bool)pix;
                texture[143-*(ram+REG_LY)][screen_x_pos][0] = pixvals[get_background_palette(pix)][0];
                texture[143-*(ram+REG_LY)][screen_x_pos][1] = pixvals[get_background_palette(pix)][1];
                texture[143-*(ram+REG_LY)][screen_x_pos][2] = pixvals[get_background_palette(pix)][2];
            }
            if (pixel_drawn) window_internal_counter++;
        }
    }
}


static inline void draw_objects(void) {
    /* Draw the object layer on the current scanline. Assumes objects are sorted by xpos, largest first */
    if (((*(ram+REG_LCDC))&2) && objects_found) { // draw objects if object layer is enabled and the current scanline contains at least one object
        uint16_t tiles[20][8];
        for (int8_t i = objects_found-1; i >= 0; i--) {
            if ((*(ram+REG_LCDC))&4) { // object 8x16 mode
                load_tile(tiles[2*i], get_tile_addr(objects[i].tileid&0xFE, 1)); // &0xFE force-resets lower bit
                load_tile(tiles[2*i+1], get_tile_addr(objects[i].tileid|0x01, 1)); // |0x01 force-sets lower bit
            } else {
                load_tile(tiles[i], get_tile_addr(objects[i].tileid, 1));
            }
            for (int16_t screen_x = objects[i].xpos-8; screen_x<objects[i].xpos; screen_x++) {
                if (screen_x >= 0 && screen_x < SCREEN_WIDTH) {
                    ObjectAttribute target_object = objects[i];
                    uint8_t sprite_x = screen_x - (target_object.xpos-8);
                    if (!target_object.xflip) sprite_x = 7 - sprite_x;
                    uint8_t sprite_y = *(ram+REG_LY) - (target_object.ypos-16);
                    if (target_object.yflip && !((*(ram+REG_LCDC))&4)) sprite_y = 7 - sprite_y;
                    if (target_object.yflip && ((*(ram+REG_LCDC))&4)) sprite_y = 15 - sprite_y;
                    uint16_t tile_line;
                    if (!((*(ram+REG_LCDC))&4)) { // object 8x8 mode
                        tile_line = tiles[i][sprite_y];
                    } else { // object 8x16 mode
                        if (sprite_y < 8) {
                            tile_line = tiles[2*i][sprite_y];
                        } else {
                            tile_line = tiles[2*i + 1][sprite_y-8];
                        }
                    }
                    uint8_t pixel = (tile_line>>(2*sprite_x))&3;
                    if (pixel && !(target_object.priority & bgw_priority_map[143-*(ram+REG_LY)][screen_x])) { // skip if transparent and background does not have priority
                        texture[143-*(ram+REG_LY)][screen_x][0] = pixvals[get_object_palette(target_object.palette, (tile_line>>(2*sprite_x))&3)][0];
                        texture[143-*(ram+REG_LY)][screen_x][1] = pixvals[get_object_palette(target_object.palette, (tile_line>>(2*sprite_x))&3)][1];
                        texture[143-*(ram+REG_LY)][screen_x][2] = pixvals[get_object_palette(target_object.palette, (tile_line>>(2*sprite_x))&3)][2];
                    }
                }
            }
        }
    }
}


static void debug_draw_background_tile(uint16_t tile[8], GLubyte tex_array[256][256][3], uint8_t base_x, uint8_t base_y) {
    /* draw a tile at a particular coordinate */
    for (int v=0; v<8; v++) {
        for (int u=0; u<8; u++) {
            uint8_t pix = (tile[u] >> (14-2*v))&3;
            tex_array[255-(base_y*8+u)][base_x*8+v][0] = pixvals[get_background_palette(pix)][0];
            tex_array[255-(base_y*8+u)][base_x*8+v][1] = pixvals[get_background_palette(pix)][1];
            tex_array[255-(base_y*8+u)][base_x*8+v][2] = pixvals[get_background_palette(pix)][2];
        }
    }
}


static void debug_draw_sprite_tile(uint16_t tile[8], uint8_t base_x, uint8_t base_y, ObjectAttribute object) {
    /* draw a tile at a particular coordinate */
    for (int v=0; v<8; v++) {
        for (int u=0; u<8; u++) {
            uint8_t pix;
            if (object.xflip) {
                pix = (tile[u] >> (2*v))&3;
            } else {
                pix = (tile[u] >> (14-2*v))&3;
            }
            uint8_t ypos;
            if (object.yflip) {
                ypos = base_y*8+u;
            } else {
                ypos = 15-(base_y*8+u);
            }
            object_tilemap[ypos][base_x*8+v][0] = pixvals[get_object_palette(object.palette, pix)][0];
            object_tilemap[ypos][base_x*8+v][1] = pixvals[get_object_palette(object.palette, pix)][1];
            object_tilemap[ypos][base_x*8+v][2] = pixvals[get_object_palette(object.palette, pix)][2];
        }
    }
}


static void debug_tilemaps(void) {
    /* draw the background and window tilespaces on the debug window */
    for (int y=0; y<32; y++) {
        for (int x=0; x<32; x++) {
            uint16_t tile[8];
            load_tile(tile, get_tile_addr(get_tile_id(y*32+x, 0), 0));
            debug_draw_background_tile(tile, bg_tilemap, x, y);

            load_tile(tile, get_tile_addr(get_tile_id(y*32+x, 1), 0));
            debug_draw_background_tile(tile, window_tilemap, x, y);
        }
    }
    uint8_t bg_scanline = *(ram+REG_LY) + *(ram+REG_SCY);

    for (int x=0; x<161; x++) {
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY))]     [(uint8_t)(*(ram+REG_SCX)+x)]   [0] = 255;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY))]     [(uint8_t)(*(ram+REG_SCX)+x)]   [1] = 0;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY))]     [(uint8_t)(*(ram+REG_SCX)+x)]   [2] = 0;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+SCREEN_HEIGHT)] [(uint8_t)(*(ram+REG_SCX)+x)]   [0] = 255;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+SCREEN_HEIGHT)] [(uint8_t)(*(ram+REG_SCX)+x)]   [1] = 0;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+SCREEN_HEIGHT)] [(uint8_t)(*(ram+REG_SCX)+x)]   [2] = 0;
    }
    for (int y=0; y<SCREEN_HEIGHT; y++) {
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+y)]   [(uint8_t)(*(ram+REG_SCX))]     [0] = 255;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+y)]   [(uint8_t)(*(ram+REG_SCX))]     [1] = 0;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+y)]   [(uint8_t)(*(ram+REG_SCX))]     [2] = 0;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+y)]   [(uint8_t)(*(ram+REG_SCX)+SCREEN_WIDTH)] [0] = 255;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+y)]   [(uint8_t)(*(ram+REG_SCX)+SCREEN_WIDTH)] [1] = 0;
        bg_tilemap[255-(uint8_t)(*(ram+REG_SCY)+y)]   [(uint8_t)(*(ram+REG_SCX)+SCREEN_WIDTH)] [2] = 0;
    }
    if (debug_scanlines) {
        for (int x=0; x<161; x++) {
            bg_tilemap[(uint8_t)(254-bg_scanline)]                  [(uint8_t)(*(ram+REG_SCX)+x)]   [0] = 0;
            bg_tilemap[(uint8_t)(254-bg_scanline)]                  [(uint8_t)(*(ram+REG_SCX)+x)]   [1] = 0;
            bg_tilemap[(uint8_t)(254-bg_scanline)]                  [(uint8_t)(*(ram+REG_SCX)+x)]   [2] = 255;
        }
    }
}


static void debug_sprites(void) {
    /* Debug util to display each sprite */

    uint16_t blank[8] = {0,0,0,0,0,0,0,0};
    for (int i=0; i<40; i++) {
        uint8_t flags = *(ram+0xFE00+(i*4)+3);
        ObjectAttribute object = (ObjectAttribute) {
            .ypos = *(ram+0xFE00+(i*4)),
            .xpos = *(ram+0xFE00+(i*4)+1),
            .tileid = *(ram+0xFE00+(i*4)+2),
            .priority = flags & (1<<7),
            .yflip = flags & (1<<6),
            .xflip = flags & (1<<5),
            .palette = flags & (1<<4)
        };
        uint16_t sprite[8];
        load_tile(sprite, get_tile_addr(object.tileid, 1));
        debug_draw_sprite_tile(sprite, i, object.yflip, object);
        if (*(ram+REG_LCDC)&4) { // 8x16 sprites
            load_tile(sprite, get_tile_addr(object.tileid+1, 1));
            debug_draw_sprite_tile(sprite, i, !object.yflip, object);
        } else {
            debug_draw_sprite_tile(blank, i, !object.yflip, object);
        }
    }
}

static void debug_tile_boundaries(void) {
    for (uint8_t x=0; x<SCREEN_WIDTH; x++) {
        if (!((*(ram + REG_LY) | x)&7)) {
            texture[143-*(ram + REG_LY)][x][0] = 255;
            texture[143-*(ram + REG_LY)][x][1] = 0;
            texture[143-*(ram + REG_LY)][x][2] = 0;
        }
    }
}


static void debug_vram(GLubyte block[32][256][3], uint16_t starting_addr) {
    /* draw vram for debugging */
    for (uint8_t i=0; i<4; i++) {
        for (uint8_t j=0; j<32; j++) {
            uint16_t tile[8];
            load_tile(tile, starting_addr + (i*32+j)*16);
            for (int v=0; v<8; v++) {
                for (int u=0; u<8; u++) {
                    uint8_t pix = (tile[v] >> (14-2*u))&3;
                    block[31-(i*8+v)][j*8+u][0] = pixvals[get_background_palette(pix)][0];
                    block[31-(i*8+v)][j*8+u][1] = pixvals[get_background_palette(pix)][1];
                    block[31-(i*8+v)][j*8+u][2] = pixvals[get_background_palette(pix)][2];
                }
            }
        }
    }
}


bool tick_graphics(void) {
    /* Main tick procedure for graphics */

    if (lcd_enable ^ (*(ram+REG_LCDC)>>7)) { //lcd state change
        if (*(ram+REG_LCDC)>>7) { // LCD was just turned on
            lcd_enable = 1;
        } else { // LCD was just turned off
            dot = 0;
            lcd_enable = 0;
            *(ram+REG_STAT) &= 0xFC;
            *(ram+REG_STAT) += 0; // set ppu mode to 0
            blank_screen();
        }
    }
    *(ram+REG_LY) = (dot/456); // LY (scanline)
    *(ram+REG_STAT) &= 0xFB;
    *(ram+REG_STAT) |= (*(ram+REG_LY) == *(ram+REG_LYC))<<2; // set LY=LYC flag

    bool current_stat_state = (
        ((*(ram+REG_STAT)&0x40)   && ((*(ram+REG_STAT)>>2)&1))   || // LY=LYC is set
        (((*(ram+REG_STAT)>>5)&1) && ((*(ram+REG_STAT)&3) == 2)) || // mode 2 is set & ppu is in mode 2
        (((*(ram+REG_STAT)>>4)&1) && ((*(ram+REG_STAT)&3) == 1)) || // mode 1 is set & ppu is in mode 1
        (((*(ram+REG_STAT)>>3)&1) && ((*(ram+REG_STAT)&3) == 0)) // mode 0 is set & ppu is in mode 0
    );
    if ((!old_stat_state) && current_stat_state) *(ram+REG_IF) |= 2; // Request a STAT interrupt
    old_stat_state = current_stat_state;

    if (lcd_enable) {
        if (dot == 65564) { // enter VBLANK
            *(ram+REG_STAT) &= 0xFC;
            *(ram+REG_STAT) += 1; // set ppu mode to 1
            *(ram+REG_IF) |= 1; // Request a VBlank interrupt
            xoffset++;
            if (xoffset == SCREEN_HEIGHT) xoffset = 0;
            window_internal_counter = 0;
            glutMainLoopEvent();
            glutPostRedisplay();
            if (debug_tilemap) {
                glutSetWindow(WindowDebug);
                debug_tilemaps();
                debug_sprites();
                debug_vram(vram_block_1, 0x8000);
                debug_vram(vram_block_2, 0x8800);
                debug_vram(vram_block_3, 0x9000);

                glutMainLoopEvent();
                glutPostRedisplay();
                glutSetWindow(WindowMain);
                debug_frames_done++;
            }
            if (frame_by_frame) {
                if (debug_frames_done >= debug_frameskip) {
                    getchar();
                    (void)! scanf("\n");
                }
            } else {
                framerate();
            }
        } else if ((*(ram+REG_LY) < SCREEN_HEIGHT) && (dot % 456) == 0) { // New scanline
            *(ram+REG_STAT) &= 0xFC;
            *(ram+REG_STAT) += 2; // set ppu mode to 2
            read_objects();
            qsort(objects, objects_found, sizeof(ObjectAttribute), compare_obj_xvalue);
        } else if ((*(ram+REG_LY) < SCREEN_HEIGHT) && (dot % 456) == 80) { // Enter drawing mode
            *(ram+REG_STAT) &= 0xFC;
            *(ram+REG_STAT) += 3; // set ppu mode to 3
            draw_background();
            draw_window();
            draw_objects();
            if (debug_scanlines) {
                debug_tile_boundaries();
            }

        } else if ((*(ram+REG_LY) < SCREEN_HEIGHT) && (dot % 456) == 232) { // Enter Hblank
            *(ram+REG_STAT) &= 0xFC; // set ppu mode to 0
            if (debug_scanlines) {
                if (debug_frames_done >= debug_frameskip) {
                    getchar();
                    (void)! scanf("\n");
                }
                glutMainLoopEvent();
                glutPostRedisplay();
                glutSetWindow(WindowDebug);
                debug_tilemaps();
                debug_sprites();
                debug_vram(vram_block_1, 0x8000);
                debug_vram(vram_block_2, 0x8800);
                debug_vram(vram_block_3, 0x9000);

                glutMainLoopEvent();
                glutPostRedisplay();
                glutSetWindow(WindowMain);
            }
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