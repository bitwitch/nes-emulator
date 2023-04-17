window_state_t nes_window, debug_window, memory_window;
SDL_GameController *controllers[2];
platform_state_t platform_state;
platform_state_t last_platform_state;
uint8_t controller_registers[2];

char scancode_to_char[] = {
    [SDL_SCANCODE_A] = 'a',
    [SDL_SCANCODE_B] = 'b',
    [SDL_SCANCODE_C] = 'c',
    [SDL_SCANCODE_D] = 'd',
    [SDL_SCANCODE_E] = 'e',
    [SDL_SCANCODE_F] = 'f',
    [SDL_SCANCODE_G] = 'g',
    [SDL_SCANCODE_H] = 'h',
    [SDL_SCANCODE_I] = 'i',
    [SDL_SCANCODE_J] = 'j',
    [SDL_SCANCODE_K] = 'k',
    [SDL_SCANCODE_L] = 'l',
    [SDL_SCANCODE_M] = 'm',
    [SDL_SCANCODE_N] = 'n',
    [SDL_SCANCODE_O] = 'o',
    [SDL_SCANCODE_P] = 'p',
    [SDL_SCANCODE_Q] = 'q',
    [SDL_SCANCODE_R] = 'r',
    [SDL_SCANCODE_S] = 's',
    [SDL_SCANCODE_T] = 't',
    [SDL_SCANCODE_U] = 'u',
    [SDL_SCANCODE_V] = 'v',
    [SDL_SCANCODE_W] = 'w',
    [SDL_SCANCODE_X] = 'x',
    [SDL_SCANCODE_Y] = 'y',
    [SDL_SCANCODE_Z] = 'z',
    [SDL_SCANCODE_1] = '1',
    [SDL_SCANCODE_2] = '2',
    [SDL_SCANCODE_3] = '3',
    [SDL_SCANCODE_4] = '4',
    [SDL_SCANCODE_5] = '5',
    [SDL_SCANCODE_6] = '6',
    [SDL_SCANCODE_7] = '7',
    [SDL_SCANCODE_8] = '8',
    [SDL_SCANCODE_9] = '9',
    [SDL_SCANCODE_0] = '0',
};


static void generate_font_glyphs(void);
static void do_keyboard_input(SDL_KeyboardEvent *event);
static void do_controller_input(SDL_ControllerButtonEvent *event);

void io_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    memset(&platform_state, 0, sizeof(platform_state));
    memset(&last_platform_state, 0, sizeof(last_platform_state));

	// default to no text input
	SDL_StopTextInput();

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

    /* Sound */
    apu_init();

    signal(SIGINT, SIG_DFL);
}

// NOTE(shaw): io_init must be called before initializing any windows
void io_init_window(window_state_t *window, char *name, int width, int height) {
	if (window->window) return; // only allow initialization once

    window->window = SDL_CreateWindow(
		name,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
		width, height,
        SDL_WINDOW_RESIZABLE);
    if (!window->window) {
        fprintf(stderr, "Failed to create window '%s': %s\n", name, SDL_GetError());
		return;
    }

    window->renderer = SDL_CreateRenderer(window->window, -1, 0);
    if (!window->renderer) {
        fprintf(stderr, "Failed to create renderer for window '%s': %s\n", name, SDL_GetError());
		return;
    }

	// Load True Type Font
	window->font = STBTTF_OpenFont(window->renderer, "c:/windows/fonts/consola.ttf", 20);

	window->width = width;
	window->height = height;
}


char text[256];
char *composition;
int cursor;
int selection_len;

/* do_input is the platform level input handler that will keep the current
 * input states up to date in platform_state */
void do_input() {
    SDL_Event event;

	// reset platform_state
	platform_state.wheel_y = 0; 
	platform_state.hex_char = 0;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
					if (event.window.windowID == SDL_GetWindowID(nes_window.window)) {
						exit(0);
					} else {
						SDL_HideWindow(SDL_GetWindowFromID(event.window.windowID));
					}
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

			case SDL_MOUSEWHEEL:
				platform_state.wheel_y = event.wheel.y;
				break;

            default:
                break;
        }
    }
}

static void do_keyboard_input(SDL_KeyboardEvent *event) {
    if (event->repeat)
        return;

	int scancode = event->keysym.scancode;

	// store hex_char
	if (event->type == SDL_KEYDOWN && 
		((scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_F) ||
		(scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_0)))
	{
		platform_state.hex_char = scancode_to_char[scancode];
	}


    switch(scancode) {
        /* emulator buttons */
        case SDL_SCANCODE_RETURN:
            platform_state.enter = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_SPACE:
            platform_state.space = event->type == SDL_KEYDOWN;
            break;
		case SDL_SCANCODE_BACKSPACE:
			platform_state.backspace = event->type == SDL_KEYDOWN;
			break;
        case SDL_SCANCODE_F:
            platform_state.f = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_W:
            platform_state.w = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_M:
            platform_state.m = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_G:
            platform_state.g = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_9:
            platform_state.nine = event->type == SDL_KEYDOWN;
            break;
        case SDL_SCANCODE_GRAVE:
            platform_state.tilde = event->type == SDL_KEYDOWN;
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
    SDL_SetRenderDrawColor(nes_window.renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(nes_window.renderer);

	if (debug_window.window) {
		SDL_SetRenderDrawColor(debug_window.renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(debug_window.renderer);
	}

	if (memory_window.window) {
		SDL_SetRenderDrawColor(memory_window.renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(memory_window.renderer);
	}
}

void io_render_sprites(void) {
    for (int i=0; i<nes_window.sprite_count; ++i) {
        sprite_t *s = nes_window.sprites[i];
        SDL_UpdateTexture(s->texture, NULL, s->pixels, s->w * sizeof(uint32_t));
		SDL_Rect *dstrect = NULL;
		if (s->dstrect.w != 0 && s->dstrect.h != 0)
			dstrect = &s->dstrect;
        SDL_RenderCopy(nes_window.renderer, s->texture, &s->srcrect, dstrect);
    }

	if (debug_window.window) {
		for (int i=0; i<debug_window.sprite_count; ++i) {
			sprite_t *s = debug_window.sprites[i];
			SDL_UpdateTexture(s->texture, NULL, s->pixels, s->w * sizeof(uint32_t));
			SDL_RenderCopy(debug_window.renderer, s->texture, &s->srcrect, &s->dstrect);
		}
	}
}

void io_render_present(void) {
    SDL_RenderPresent(nes_window.renderer);
	if (debug_window.window)
		SDL_RenderPresent(debug_window.renderer);
	if (memory_window.window)
		SDL_RenderPresent(memory_window.renderer);
}

/*
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
*/

/* a subsprite renders a rectangle out of a potentially larger texture */
sprite_t make_sub_sprite(window_state_t *window, uint32_t *pixels, int w, int h, 
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
    s.texture = SDL_CreateTexture(window->renderer,
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

sprite_t make_sprite(window_state_t *window, uint32_t *pixels, int w, int h, 
                     int dest_x, int dest_y, int dest_w, int dest_h) 
{
    return make_sub_sprite(
        window, pixels, w, h, 
        0, 0, dest_w, dest_h,
        dest_x, dest_y, dest_w, dest_h);
}

// TODO(shaw): after adding multiple windows and refactoring window things into
// window_state_t, it seems that "registering" sprites could be done directly
// in make_sprite. Then it would have to allocate memory for sprites rather than
// copying on the stack. 
void register_sprite(window_state_t *window, sprite_t *sprite) {
	assert(window->sprite_count < MAX_WINDOW_SPRITES);
	window->sprites[window->sprite_count++] = sprite;
}

void render_text(window_state_t *window, char *text, int x, int y) {
	if (!window->window) return;
	SDL_SetRenderDrawColor(window->renderer, 255, 255, 255, 255);
	STBTTF_RenderText(window->renderer, window->font, (float)x, (float)y, text);

	// SDL_Renderer *rend = (wid == WIN_DEBUG) ? debug_renderer : renderer;
    // int i; char *c;
    // for (i=0, c=text; *c != '\0'; ++c, ++i) {
        // assert(*c > 31 && *c < 127);
        // SDL_Rect dstrect = { 
            // x + i*FONT_CHAR_WIDTH*FONT_SCALE, 
            // y, 
            // FONT_CHAR_WIDTH*FONT_SCALE, 
            // FONT_CHAR_HEIGHT*FONT_SCALE };
        // SDL_RenderCopy(rend, font.texture, &font.glyphs[*c-32], &dstrect);
    // }

}

void render_text_color(window_state_t *window, char *text, int x, int y, uint32_t color) {
	if (!window->window) return;
	uint8_t r, g, b;
	r = (color >> 16) & 0xFF;
	g = (color >> 8)  & 0xFF;
	b = (color >> 0)  & 0xFF;
	SDL_SetRenderDrawColor(window->renderer, r, g, b, 255);
	STBTTF_RenderText(window->renderer, window->font, (float)x, (float)y, text);

	// SDL_Renderer *rend = (wid == WIN_DEBUG) ? debug_renderer : renderer;
    // int i; char *c;
    // for (i=0, c=text; *c != '\0'; ++c, ++i) {
        // assert(*c > 31 && *c < 127);
        // SDL_Rect dstrect = { 
            // x + i*FONT_CHAR_WIDTH*FONT_SCALE, 
            // y, 
            // FONT_CHAR_WIDTH*FONT_SCALE, 
            // FONT_CHAR_HEIGHT*FONT_SCALE };
        // SDL_SetTextureColorMod(font.texture, 
            // (color >> 16) & 0xFF,
            // (color >>  8) & 0xFF,
            // (color >>  0) & 0xFF);
        // SDL_RenderCopy(rend, font.texture, &font.glyphs[*c-32], &dstrect);
        // SDL_SetTextureColorMod(font.texture, 0xFF, 0xFF, 0xFF);
    // }
}

void io_deinit(void) {
}

