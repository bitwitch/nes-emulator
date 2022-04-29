#ifndef __IO_H__
#define __IO_H__

#include <SDL2/SDL.h>
#include <stdint.h>

#define WIDTH  256
#define HEIGHT 240
#define SCALE 3
#define WINDOW_WIDTH  (WIDTH*SCALE/0.68)
#define WINDOW_HEIGHT (HEIGHT*SCALE)

typedef struct {
    SDL_Texture *texture;
    uint32_t *pixels;
    SDL_Rect dstrect;
} sprite_t;

uint32_t *io_init(void);
void io_deinit(void);
void draw(void);
uint64_t get_ticks(void);
void do_input(void);
sprite_t make_sprite(uint32_t *pixels, int dest_x, int dest_y, int dest_w, int dest_h);

#endif
