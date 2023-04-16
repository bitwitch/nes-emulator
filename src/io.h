#ifndef __IO_H__
#define __IO_H__

/* PPU_WIDTH and HEIGHT is size output by the ppu */
#define PPU_WIDTH  256
#define PPU_HEIGHT 240
/* NES_WIDTH and HEIGHT is size of section output by the ppu that is actually visible */
#define NES_WIDTH  256
#define NES_HEIGHT 224
#define SCALE 3
#define WINDOW_WIDTH  (NES_WIDTH*SCALE)
#define WINDOW_HEIGHT (NES_HEIGHT*SCALE)
#define DEBUG_WINDOW_WIDTH  (NES_WIDTH*SCALE*0.66)
#define DEBUG_WINDOW_HEIGHT (NES_HEIGHT*SCALE)
#define MEMORY_WINDOW_WIDTH  800
#define MEMORY_WINDOW_HEIGHT 600

#define FONT_CHAR_WIDTH 7
#define FONT_CHAR_HEIGHT 9
#define FONT_SCALE 2

#define MAX_WINDOW_SPRITES 16

typedef struct {
    SDL_Texture *texture;
    SDL_Surface *surface;
    uint32_t *pixels;
    int w, h;
    SDL_Rect srcrect, dstrect;
} sprite_t;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	STBTTF_Font* font;
	sprite_t *sprites[MAX_WINDOW_SPRITES];
	int sprite_count;
	bool hidden; 
} window_state_t; 

typedef struct {
    bool enter;
    bool space;
    bool escape;
    bool f,w,m;
    bool nine;
    bool tilde;

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

extern window_state_t nes_window, debug_window, memory_window;
extern platform_state_t platform_state;
extern platform_state_t last_platform_state;
extern uint8_t controller_registers[2]; /* parallel to serial shift registers */

void io_init(void);
void io_deinit(void);
void io_init_debug_window(void);
void io_destroy_debug_window(void);
void io_render_prepare(void);
void io_render_sprites(void);
void io_render_present(void);
uint64_t get_ticks(void);
void controller_write(int controller_index, uint8_t data);
uint8_t controller_read(int controller_index);
void do_input();
sprite_t make_sprite(window_state_t *window, uint32_t *pixels, int w, int h, 
                     int dest_x, int dest_y, int dest_w, int dest_h);
sprite_t make_sub_sprite(window_state_t *window, uint32_t *pixels, int w, int h, 
                         int src_x, int src_y, int src_w, int src_h,
                         int dest_x, int dest_y, int dest_w, int dest_h);
void register_sprite(window_state_t *window, sprite_t *sprite);
void render_text(window_state_t *window, char *text, int x, int y);
void render_text_color(window_state_t *window, char *text, int x, int y, uint32_t color_mod);


#endif
