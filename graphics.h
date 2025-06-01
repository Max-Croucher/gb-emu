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

void framerate(void);
void gl_tick(void);
void object_map_draw_quad(uint8_t i);
void draw_text(float x, float y, void *font, const char* string);
void gl_tick_debug_window(void);
void reshape(int w, int h);
void init_graphics(int *argc, char *argv[], char rom_title[16]);
void window_closed(void);
void key_pressed (unsigned char key, int x, int y);
void key_released (unsigned char key, int x, int y);
void blank_screen(void);
uint16_t interleave(uint16_t a, uint16_t b);
uint8_t get_tile_id(uint16_t tilepos, bool is_window_layer);
uint16_t get_tile_addr(uint8_t tile_id, bool is_object);
void load_tile(uint16_t tile_data[8], uint16_t tile_addr);
void read_objects(void);
uint8_t get_background_palette(uint8_t palette_value);
uint8_t get_object_palette(bool palette_id, uint8_t palette_value);
int compare_obj_xvalue(const void *a, const void *b);
void draw_background(void);
void draw_window(void);
void draw_objects(void);
bool tick_graphics(void);
void debug_draw_background_tile(uint16_t tile[8], GLubyte tex_array[256][256][3], uint8_t base_x, uint8_t base_y);
void debug_draw_sprite_tile(uint16_t tile[8], uint8_t base_x, uint8_t base_y, ObjectAttribute object);
void debug_tilemaps(void);
void debug_sprites(void);






#endif // GRAPHICS_H