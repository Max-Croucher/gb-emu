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

void init_screen_tex(void);
void gl_tick(void);
void reshape(int w, int h);
void init_graphics(int *argc, char *argv[]);
void window_closed(void);
void key_pressed (unsigned char key, int x, int y);
void key_released (unsigned char key, int x, int y);
void blank_screen(void);
uint16_t interleave(uint16_t a, uint16_t b);
uint8_t get_tile_id(uint16_t tilepos, bool is_window_layer);
uint16_t get_tile_addr(uint8_t tile_id, bool is_object);
void load_tile(uint16_t tile_data[8], uint16_t tile_addr);
uint8_t read_objects(ObjectAttribute attrbank[10]);
uint8_t get_background_palette(uint8_t palette_value);
uint8_t get_object_palette(bool palette_id, uint8_t palette_value);
int compare_obj_xvalue(const void *a, const void *b);
void draw_background();
void draw_window();
void draw_objects(ObjectAttribute objects[10], uint8_t objects_found);
bool tick_graphics(void);
void print_tilemaps(void);






#endif // GRAPHICS_H