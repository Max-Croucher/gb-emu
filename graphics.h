/* Header file for graphics.c, controlling gameboy graphics
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

typedef struct {
    uint8_t ypos;
    uint8_t xpos;
    uint8_t tileid;
    bool priority;
    bool yflip;
    bool xflip;
    bool palette;
} ObjectAttribute;

void gl_tick(void);
void gl_tick_debug_window(void);
void reshape_window_ratio(int w, int h);
void reshape_window_free(int w, int h);
void init_graphics(int *argc, char *argv[], char rom_title[16]);
void window_closed(void);
void take_screenshot(char *filename);
void key_pressed (unsigned char key, int x, int y);
void key_released (unsigned char key, int x, int y);
bool tick_graphics(void);

#endif // GRAPHICS_H