#include <assert.h>
#include <signal.h>
#include "io.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static SDL_Window *window;
static SDL_Renderer *renderer;

#define MAX_SPRITES 16
static sprite_t *sprites[MAX_SPRITES];
static int sprite_count;

static struct {
    int w, h, bytes_per_pixel;
    SDL_Texture *texture;
    SDL_Rect glyphs[95];
} font;

void generate_font_glyphs(void);

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

    uint8_t *font_pixels = stbi_load("font.png", &font.w, &font.h, &font.bytes_per_pixel, STBI_rgb);
    if (!font_pixels)
        fprintf(stderr, "Failed to load font: font.png\n");
    printf("Loaded font.png: w=%d h=%d, bytes_per_pixel=%d\n", font.w, font.h, font.bytes_per_pixel);

    SDL_Surface *font_surface = SDL_CreateRGBSurfaceFrom(
        font_pixels,
        font.w,
        font.h,
        8*font.bytes_per_pixel,
        3*font.w,
        0, 0, 0, 0);
    if (!font_surface) {
        fprintf(stderr, "Creating surface failed: %s", SDL_GetError());
        exit(1);
    }

    font.texture = SDL_CreateTextureFromSurface(renderer, font_surface);

    SDL_FreeSurface(font_surface);
    stbi_image_free(font_pixels);

    generate_font_glyphs();

    signal(SIGINT, SIG_DFL);
}

void do_input(platform_state_t *platform_state) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            /*game.quit = true;*/
            exit(0);
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (event.key.repeat == 0) {
                switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_RETURN:
                    platform_state->enter = event.type == SDL_KEYDOWN;
                    break;
                case SDL_SCANCODE_SPACE:
                    platform_state->space = event.type == SDL_KEYDOWN;
                    break;
                case SDL_SCANCODE_F:
                    platform_state->f = event.type == SDL_KEYDOWN;
                    break;
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

void prepare_drawing(void) {
    SDL_RenderClear(renderer);
}

void render_sprites(void) {
    for (int i=0; i<sprite_count; ++i) {
        sprite_t *s = sprites[i];
        SDL_UpdateTexture(s->texture, NULL, s->pixels, s->w * sizeof(uint32_t));
        SDL_RenderCopy(renderer, s->texture, NULL, &s->dstrect);
    }

}

void draw(void) {
    SDL_RenderPresent(renderer);
}

void generate_font_glyphs(void) {
    for (int i=0; i<95; ++i) {
        int col = i % (font.w/FONT_CHAR_WIDTH);
        int row = i / (font.w/FONT_CHAR_WIDTH);

        font.glyphs[i] = (SDL_Rect) {
            .x = col*FONT_CHAR_WIDTH,
            .y = row*FONT_CHAR_HEIGHT,
            .w = FONT_CHAR_WIDTH,
            .h = FONT_CHAR_HEIGHT
        };
    }
}

sprite_t make_sprite(uint32_t *pixels, int w, int h, int dest_x, int dest_y, int dest_w, int dest_h) {
    sprite_t s;
    s.pixels = pixels;
    s.surface = SDL_CreateRGBSurfaceFrom(
        pixels,
        w, h,
        8*3,      /* depth */
        w,        /* pitch */
        0x0000FF, 
        0x00FF00, 
        0xFF0000, 
        0);
    if (!s.surface) {
        fprintf(stderr, "Creating surface failed: %s", SDL_GetError());
        exit(1);
    }
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

void render_text(char *text, int x, int y) {
    int i; char *c;
    for (i=0, c=text; *c != '\0'; ++c, ++i) {
        assert(*c > 31 && *c < 127);
        SDL_Rect dstrect = { 
            x + i*FONT_CHAR_WIDTH*SCALE, 
            y, 
            FONT_CHAR_WIDTH*SCALE, 
            FONT_CHAR_HEIGHT*SCALE };
        SDL_RenderCopy(renderer, font.texture, &font.glyphs[*c-32], &dstrect);
    }

}

void set_font_color(uint32_t color) {
    SDL_SetTextureColorMod(font.texture,
        (color >> 16) & 0xFF,
        (color >>  8) & 0xFF,
        (color >>  0) & 0xFF);
}

void render_text_color(char *text, int x, int y, uint32_t color) {
    int i; char *c;
    for (i=0, c=text; *c != '\0'; ++c, ++i) {
        assert(*c > 31 && *c < 127);
        SDL_Rect dstrect = { 
            x + i*FONT_CHAR_WIDTH*SCALE, 
            y, 
            FONT_CHAR_WIDTH*SCALE, 
            FONT_CHAR_HEIGHT*SCALE };
        SDL_SetTextureColorMod(font.texture, 
            (color >> 16) & 0xFF,
            (color >>  8) & 0xFF,
            (color >>  0) & 0xFF);
        SDL_RenderCopy(renderer, font.texture, &font.glyphs[*c-32], &dstrect);
        SDL_SetTextureColorMod(font.texture, 0xFF, 0xFF, 0xFF);
    }
}

void io_deinit(void) {
}


