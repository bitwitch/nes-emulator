#include <SDL2/SDL.h>
#include <signal.h>

#include "io.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;
static uint32_t     *pixels;

uint32_t *io_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(
        "NES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        3*WIDTH, 3*HEIGHT,
        SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    pixels = malloc(WIDTH * HEIGHT * sizeof(uint32_t)); /* LEAK */
    if (!pixels) {
        perror("malloc");
        exit(1);
    }

    texture = SDL_CreateTexture(renderer,
       SDL_PIXELFORMAT_ARGB8888,
       SDL_TEXTUREACCESS_STREAMING,
       WIDTH, HEIGHT);
    if (!texture) {
        fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    signal(SIGINT, SIG_DFL);

    return pixels;
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
    /*  upload pixel data to GPU */
    SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void io_deinit(void) {
    if (pixels) free(pixels);
}


/* TODO(shaw): put_pixel function */

