#ifndef __IO_H__
#define __IO_H__

#include <SDL2/SDL.h>
#include <stdint.h>

#define NES_WIDTH  256
#define NES_HEIGHT 240
#define SCALE 3
#define NES_DEBUG_RATIO (0.68) /* how much of screen is nes vs how much is debug info */
#define WINDOW_WIDTH  (NES_WIDTH*SCALE/NES_DEBUG_RATIO)
#define WINDOW_HEIGHT (NES_HEIGHT*SCALE)
#define FONT_CHAR_WIDTH 7
#define FONT_CHAR_HEIGHT 9

typedef struct {
    SDL_Texture *texture;
    SDL_Surface *surface;
    uint32_t *pixels;
    int w, h;
    SDL_Rect dstrect;
} sprite_t;

void io_init(void);
void io_deinit(void);
void prepare_drawing(void);
void render_sprites(void);
void draw(void);
uint64_t get_ticks(void);
void do_input(void);
sprite_t make_sprite(uint32_t *pixels, int w, int h,
                     int dest_x, int dest_y, int dest_w, int dest_h);
void register_sprite(sprite_t *sprite);
void render_text(char *text, int x, int y);

#endif
