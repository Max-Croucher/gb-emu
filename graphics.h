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

void init_screen_tex(GLubyte texture[144][160][3]);
void gl_tick(void);
void reshape(int w, int h);
void init_graphics(int *argc, char *argv[]);
uint16_t interleave(uint16_t a, uint16_t b);
uint16_t get_tile_addr(uint8_t tile_id, bool is_object);
void load_tile(uint16_t tile_data[8], uint16_t tile_addr);
uint8_t read_objects(ObjectAttribute attrbank[10], uint8_t scanline);
void draw_background(uint8_t scanline);
void draw_window(uint8_t scanline);
void draw_objects(uint8_t scanline, ObjectAttribute objects[10], uint8_t objects_found);
bool tick_graphics(void);
void print_tilemaps(void);






#endif // GRAPHICS_H