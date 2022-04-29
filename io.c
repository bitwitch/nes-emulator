#include <assert.h>
#include <signal.h>

#include "io.h"

static SDL_Window *window;
static SDL_Renderer *renderer;

#define MAX_SPRITES 16
static sprite_t *sprites[MAX_SPRITES];
static int sprite_count;

void io_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(
        "NES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    signal(SIGINT, SIG_DFL);
}

void do_input(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            /*game.quit = true;*/
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (event.key.repeat == 0) {
                switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_UP:
                    /*game.up = event.type == SDL_KEYDOWN;*/
                    break;
                case SDL_SCANCODE_DOWN:
                    /*game.down = event.type == SDL_KEYDOWN;*/
                    break;
                case SDL_SCANCODE_LEFT:
                    /*game.left = event.type == SDL_KEYDOWN;*/
                    break;
                case SDL_SCANCODE_RIGHT:
                    /*game.right = event.type == SDL_KEYDOWN;*/
                    break;
                case SDL_SCANCODE_RETURN:
                    /*game.enter = event.type == SDL_KEYDOWN;*/
                    printf("ENTER\n");
                    break;
                default:
                    break;
                }

            }
            break;

        default:
            break;
        }
    }
}

uint64_t get_ticks(void) {
    return SDL_GetTicks64();
}

void draw(void) {
    SDL_RenderClear(renderer);
    for (int i=0; i<sprite_count; ++i) {
        sprite_t *s = sprites[i];
        SDL_UpdateTexture(s->texture, NULL, s->pixels, s->w * sizeof(uint32_t));
        SDL_RenderCopy(renderer, s->texture, NULL, &s->dstrect);
    }
    SDL_RenderPresent(renderer);
}

sprite_t make_sprite(uint32_t *pixels, int w, int h, int dest_x, int dest_y, int dest_w, int dest_h) {
    sprite_t s;
    s.pixels = pixels;
    s.texture = SDL_CreateTexture(renderer,
           SDL_PIXELFORMAT_RGB888,
           SDL_TEXTUREACCESS_STREAMING,
           w, h);
    if (!s.texture) {
        fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }
    s.w = w; 
    s.h = h;
    s.dstrect = (SDL_Rect){ dest_x, dest_y, dest_w, dest_h };
    return s;
}

void register_sprite(sprite_t *sprite) {
    assert(sprite_count < MAX_SPRITES);
    sprites[sprite_count++] = sprite;
}

