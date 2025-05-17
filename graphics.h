/* Header file for graphics.c, controlling gameboy graphics
  Author: Max Croucher
  Email: mpccroucher@gmail.com
  May 2025
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

void init_screen_tex(GLubyte texture[144][160][3]);
void gl_tick(void);
void reshape(int w, int h);
void init_graphics(int *argc, char *argv[]);
void tick_graphics(uint8_t *ram);






#endif // GRAPHICS_H