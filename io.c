#include <signal.h>

#include "io.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static sprite_t nes_quad;

/* hack to get pattern tables and palettes on screen */
extern sprite_t palettes[8];
extern sprite_t pattern_tables[2];


uint32_t *io_init(void) {
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

    nes_quad.pixels = malloc(WIDTH * HEIGHT * sizeof(uint32_t));
    if (!nes_quad.pixels) {
        perror("malloc");
        exit(1);
    }

    nes_quad.texture = SDL_CreateTexture(renderer,
       SDL_PIXELFORMAT_ARGB8888,
       SDL_TEXTUREACCESS_STREAMING,
       WIDTH, HEIGHT);
    if (!nes_quad.texture) {
        fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    nes_quad.dstrect.x = 0;
    nes_quad.dstrect.y = 0;
    nes_quad.dstrect.w = WIDTH*SCALE;
    nes_quad.dstrect.h = HEIGHT*SCALE;

    /* this is a quick hack to see the pattern tables and palettes */
    for (int i=0; i<2; ++i) {
        pattern_tables[i].texture = SDL_CreateTexture(renderer,
           SDL_PIXELFORMAT_ARGB8888,
           SDL_TEXTUREACCESS_STREAMING,
           128, 128);
        if (!pattern_tables[i].texture) {
            fprintf(stderr, "Failed to create texture for pattern tables: %s\n", SDL_GetError());
            /*exit(1);*/
        }
        pattern_tables[i].dstrect = (SDL_Rect){i*(128+3), 0, 128, 128};
    }

    for (int i=0; i<8; ++i) {
        palettes[i].texture = SDL_CreateTexture(renderer,
           SDL_PIXELFORMAT_ARGB8888,
           SDL_TEXTUREACCESS_STREAMING,
           4, 1);
        if (!palettes[i].texture) {
            fprintf(stderr, "Failed to create texture for palettes: %s\n", SDL_GetError());
            /*exit(1);*/
        }
        palettes[i].dstrect = (SDL_Rect){i*(4+1)*10, 200, 4*10, 1*10};
    }

    signal(SIGINT, SIG_DFL);

    return nes_quad.pixels;
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

    /* draw nes */
    SDL_UpdateTexture(nes_quad.texture, NULL, nes_quad.pixels, WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(renderer, nes_quad.texture, NULL, &nes_quad.dstrect);

    /* draw pattern tables */
    for (int i=0; i<2; ++i) {
        sprite_t pat = pattern_tables[i];
        SDL_UpdateTexture(pat.texture, NULL, pat.pixels, 128 * sizeof(uint32_t));
        SDL_RenderCopy(renderer, pat.texture, NULL, &pat.dstrect);
    }

    /* draw palettes */
    for (int i=0; i<8; ++i) {
        sprite_t pal = palettes[i];
        SDL_UpdateTexture(pal.texture, NULL, pal.pixels, 4 * sizeof(uint32_t));
        SDL_RenderCopy(renderer, pal.texture, NULL, &pal.dstrect);
    }

    SDL_RenderPresent(renderer);
}

void io_deinit(void) {
    if (nes_quad.pixels) free(nes_quad.pixels);
}


/* TODO(shaw): put_pixel function */

