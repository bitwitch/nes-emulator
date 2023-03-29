#include <assert.h>
#include <signal.h>
#include "io.h"
#include "apu.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static struct {
    int w, h, bytes_per_pixel;
    SDL_Texture *texture;
    SDL_Rect glyphs[95];
} font;


static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_GameController *controllers[2];

#define MAX_SPRITES 16
static sprite_t *sprites[MAX_SPRITES];
static int sprite_count;

platform_state_t platform_state;
platform_state_t last_platform_state;
uint8_t controller_registers[2];

static void generate_font_glyphs(void);
static void do_keyboard_input(SDL_KeyboardEvent *event);
static void do_controller_input(SDL_ControllerButtonEvent *event);


void io_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(
        "NES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        (int)WINDOW_WIDTH, (int)WINDOW_HEIGHT,
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

    memset(&platform_state, 0, sizeof(platform_state));
    memset(&last_platform_state, 0, sizeof(last_platform_state));

    /* Controllers */
    int num_controllers = SDL_NumJoysticks();

    if (num_controllers == 0)
        printf("No controllers connected\n");

    if (num_controllers > 0) {
        if (SDL_IsGameController(0))
            controllers[0] = SDL_GameControllerOpen(0);
        if (controllers[0])
            printf("Opened controller 0: %s\n", SDL_GameControllerNameForIndex(0));
        else 
            printf("Error: Failed to open controller 0\n");
    }

    if (num_controllers > 1) {
        if (SDL_IsGameController(1))
            controllers[1] = SDL_GameControllerOpen(1);
        if (controllers[1])
            printf("Opened controller 1: %s\n", SDL_GameControllerNameForIndex(1));
        else 
            printf("Error: Failed to open controller 1\n");
    }

    /* Load Bitmap Font */
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

    /* Sound */
    apu_init();

    signal(SIGINT, SIG_DFL);
}

/* TODO(shaw): platform_state is now global, doesn't need to be passed in here */

/* do_input is the platform level input handler that will keep the current
 * input states up to date in platform_state */
void do_input() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                        /*&& event.window.windowID == SDL_GetWindowID(window)) */
                {
                    exit(0);
                }
                break;

            case SDL_QUIT:
                /*game.quit = true;*/
                exit(0);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                do_keyboard_input(&event.key);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                do_controller_input(&event.cbutton);
                break;

            default:
                break;
        }
    }
}

static void do_keyboard_input(SDL_KeyboardEvent *event) {
    if (event->repeat)
        return;

    switch(event->keysym.scancode) {
        /* emulator buttons */
        case SDL_SCANCODE_RETURN:
            platform_state.enter = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_SPACE:
            platform_state.space = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_F:
            platform_state.f = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_W:
            platform_state.w = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_9:
            platform_state.nine = event->type == SDL_KEYDOWN;
            break;

        /* nes controller buttons */
        case SDL_SCANCODE_UP:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_UP : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_UP) | mask;
            break;
        } 
        case SDL_SCANCODE_DOWN:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_DOWN : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_DOWN) | mask;
            break;
        } 
        case SDL_SCANCODE_LEFT:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_LEFT : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_LEFT) | mask;
            break;
        } 
        case SDL_SCANCODE_RIGHT:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_RIGHT : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_RIGHT) | mask;
            break;
        } 
        case SDL_SCANCODE_S:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_SELECT : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_SELECT) | mask;
            break;
        } 
        case SDL_SCANCODE_D:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_START : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_START) | mask;
            break;
        } 
        case SDL_SCANCODE_C:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_A : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_A) | mask;
            break;
        } 
        case SDL_SCANCODE_X:
        {
            uint8_t mask = event->type == SDL_KEYDOWN ? CONTROLLER_B : 0;
            platform_state.controller_states[0] = 
                (platform_state.controller_states[0] & ~CONTROLLER_B) | mask;
            break;
        } 
        default:
            break;
    }
}

/* controller input is based on the xbox 360 controller */
static void do_controller_input(SDL_ControllerButtonEvent *event) {
    int con_id = event->which;
    switch (event->button) 
    {
        case SDL_CONTROLLER_BUTTON_A:
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_A : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_A) | mask;
            break;
        }
        case SDL_CONTROLLER_BUTTON_X: /*xbox x button maps well to nes b button */
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_B : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_B) | mask;
            break;
        }
        case SDL_CONTROLLER_BUTTON_BACK: 
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_SELECT : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_SELECT) | mask;
            break;
        } 
        case SDL_CONTROLLER_BUTTON_START: 
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_START : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_START) | mask;
            break;
        } 
        case SDL_CONTROLLER_BUTTON_DPAD_UP: 
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_UP : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_UP) | mask;
            break;
        } 
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: 
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_DOWN : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_DOWN) | mask;
            break;
        } 
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: 
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_LEFT : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_LEFT) | mask;
            break;
        } 
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: 
        {
            uint8_t mask = event->type == SDL_CONTROLLERBUTTONDOWN ? CONTROLLER_RIGHT : 0;
            platform_state.controller_states[con_id] = 
                (platform_state.controller_states[con_id] & ~CONTROLLER_RIGHT) | mask;
            break;
        } 
        default:
            break;
    }
}


void controller_write(int controller_index, uint8_t data) {
    assert(controller_index == 0 || controller_index == 1);
    if (data & 1) {
        /* 1 means continually poll controller for current button states, since
         * this is already done every frame by do_input, we do nothing here */
    } else {
        /* 0 means switch to serial mode, and we simulate this by making a copy
         * of the button states when a 0 is written, that can be used as a
         * shift register */
        controller_registers[controller_index] = platform_state.controller_states[controller_index];
    }
}

uint8_t controller_read(int controller_index) {
    uint8_t result = (controller_registers[controller_index] >> 7) & 1;
    controller_registers[controller_index] <<= 1;
    return result;
}

uint64_t get_ticks(void) {
    return SDL_GetTicks64();
}

void io_render_prepare(void) {
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);
}

void io_render_sprites(void) {
    for (int i=0; i<sprite_count; ++i) {
        sprite_t *s = sprites[i];
        SDL_UpdateTexture(s->texture, NULL, s->pixels, s->w * sizeof(uint32_t));
        SDL_RenderCopy(renderer, s->texture, &s->srcrect, &s->dstrect);
    }
}

void io_render_present(void) {
    SDL_RenderPresent(renderer);
}

static void generate_font_glyphs(void) {
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


/* a subsprite renders a rectangle out of a potentially larger texture */
sprite_t make_sub_sprite(uint32_t *pixels, int w, int h, 
                     int src_x, int src_y, int src_w, int src_h,
                     int dest_x, int dest_y, int dest_w, int dest_h) 
{
    sprite_t s;
    s.pixels = pixels;
    s.surface = SDL_CreateRGBSurfaceFrom(
        pixels,
        w, h,
        8 * sizeof(*pixels),  /* depth */
        w * sizeof(*pixels),  /* pitch */
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
    s.srcrect = (SDL_Rect){ src_x, src_y, src_w, src_h };
    s.dstrect = (SDL_Rect){ dest_x, dest_y, dest_w, dest_h };
    return s;
}

sprite_t make_sprite(uint32_t *pixels, int w, int h, 
                     int dest_x, int dest_y, int dest_w, int dest_h) 
{
    return make_sub_sprite(
        pixels, w, h, 
        0, 0, dest_w, dest_h,
        dest_x, dest_y, dest_w, dest_h);
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


