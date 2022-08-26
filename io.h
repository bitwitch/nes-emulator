#ifndef __IO_H__
#define __IO_H__

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

/* PPU_WIDTH and HEIGHT is size output by the ppu */
#define PPU_WIDTH  256
#define PPU_HEIGHT 240
/* NES_WIDTH and HEIGHT is size of section output by the ppu that is actually visible */
#define NES_WIDTH  256
#define NES_HEIGHT 224
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
    SDL_Rect srcrect, dstrect;
} sprite_t;

typedef struct {
    bool enter;
    bool space;
    bool escape;
    bool f,w;

    uint8_t controller_states[2];
} platform_state_t;

typedef enum {
    CONTROLLER_A      = 1 << 7,
    CONTROLLER_B      = 1 << 6,
    CONTROLLER_SELECT = 1 << 5,
    CONTROLLER_START  = 1 << 4,
    CONTROLLER_UP     = 1 << 3,
    CONTROLLER_DOWN   = 1 << 2,
    CONTROLLER_LEFT   = 1 << 1,
    CONTROLLER_RIGHT  = 1 << 0
} controller_buttons_t;

extern platform_state_t platform_state;
extern platform_state_t last_platform_state;
extern uint8_t controller_registers[2]; /* parallel to serial shift registers */

void io_init(void);
void io_deinit(void);
void io_render_prepare(void);
void io_render_sprites(void);
void io_render_present(void);
uint64_t get_ticks(void);
void controller_write(int controller_index, uint8_t data);
uint8_t controller_read(int controller_index);
void do_input();
sprite_t make_sprite(uint32_t *pixels, int w, int h, 
                     int dest_x, int dest_y, int dest_w, int dest_h);
sprite_t make_sub_sprite(uint32_t *pixels, int w, int h, 
                         int src_x, int src_y, int src_w, int src_h,
                         int dest_x, int dest_y, int dest_w, int dest_h);
void register_sprite(sprite_t *sprite);
void set_font_color(uint32_t color);
void render_text(char *text, int x, int y);
void render_text_color(char *text, int x, int y, uint32_t color_mod);

void render_sound_wave(float *audio_buffer, int len);

#endif
