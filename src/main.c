#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// testing this out --------------------
#define STBTTF_IMPLEMENTATION
#include "stbttf.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

// -------------------------------------


#include "cpu_6502.h"
#include "bus.h"
#include "mappers.h"
#include "cart.h"
#include "io.h"
#include "ppu.h"
#include "apu.h"

#include "common.c"
#include "cpu_6502.c"
#include "bus.c"
#include "mappers.c"
#include "cart.c"
#include "io.c"
#include "ppu.c"
#include "apu.c"

#define MS_PER_FRAME (1000/60)
#define MAX_CPU_STATE_LINES 36
#define MAX_DEBUG_LINE_CHARS 34
#define MAX_CODE_LINES 14


typedef enum {
    EM_RUN,
    EM_STEP_INSTRUCTION,
    EM_STEP_FRAME,
} emulation_mode_t;

#ifdef DEBUG_LOG
extern FILE *logfile;
#endif

static char *cpu_state_lines[MAX_CPU_STATE_LINES];
static sprite_t pattern_tables[2];
static sprite_t palettes[8];
static uint64_t elapsed_time, last_frame_time;
static bool frame_prepared;


char **get_dasm_lines(Arena *arena, uint16_t pc);
void init_debug_chr_viewer(sprite_t pattern_tables[2], sprite_t palettes[8]);
void render_memory_window(void);
void render_debug_window(Arena *arena, cpu_t *cpu);
void emulation_mode_run(cpu_t *cpu);
void emulation_mode_step_instruction(cpu_t *cpu);
void emulation_mode_step_frame(cpu_t *cpu);
void update_memory_window(void);

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s ROM_FILE\n", argv[0]);
        exit(1);
    }

    cpu_t cpu;
    read_rom_file(argv[1]);
    io_init();
	io_init_window(&nes_window, "NES", (int)WINDOW_WIDTH, (int)WINDOW_HEIGHT);


    /* LEAK: 
     * All of the malloc calls for sprite pixels are leaking. since the memory
     * is used for the lifetime of the application, we are just ignoring and
     * letting the operating system clean up at exit */

    for (int i=0; i<MAX_CPU_STATE_LINES-1; ++i) {
        cpu_state_lines[i] = calloc(MAX_DEBUG_LINE_CHARS+1, 1);
        if (!cpu_state_lines[i]) { perror("calloc"); exit(1); }
    }
    cpu_state_lines[MAX_CPU_STATE_LINES-1] = NULL;

    uint32_t *pixels = malloc(PPU_WIDTH*PPU_HEIGHT*sizeof(uint32_t));
    if (!pixels) { perror("malloc"); exit(1); }
    int vertical_overscan = (int)(0.5*(PPU_HEIGHT - NES_HEIGHT));
    int horizontal_overscan = (int)(0.5*(PPU_WIDTH - NES_WIDTH));
    sprite_t nes_quad = make_sub_sprite(&nes_window, pixels, 
		PPU_WIDTH, PPU_HEIGHT, 
        horizontal_overscan, vertical_overscan, 
		NES_WIDTH, NES_HEIGHT,
        0, 0, 0, 0); // all zeros means use entire render target
    register_sprite(&nes_window, &nes_quad);

    ppu_init(nes_quad.pixels);

    cpu_reset(&cpu);

#ifdef DEBUG_LOG
    logfile = fopen("nestest.log", "w");
#endif

	Arena frame_arena = {0};
	arena_grow(&frame_arena, ARENA_BLOCK_SIZE); // initialize so that we can call arena_get_pos()
	init_cached_ins_addrs(&cpu);

    frame_prepared = false;
    elapsed_time = 0;
    last_frame_time = get_ticks();
    emulation_mode_t emulation_mode = EM_RUN;

    for (;;) {
		char *pos = arena_get_pos(&frame_arena);

        last_platform_state = platform_state;
        do_input();
	
		// activate debug window
		if (platform_state.tilde && !last_platform_state.tilde) {
			if (!debug_window.window) {
				io_init_window(&debug_window, "NES Debug", (int)DEBUG_WINDOW_WIDTH, (int)DEBUG_WINDOW_HEIGHT);
				init_debug_chr_viewer(pattern_tables, palettes);
				update_pattern_tables(0, pattern_tables);
				update_palettes(palettes);
			} else {
				SDL_ShowWindow(debug_window.window);
				SDL_RaiseWindow(debug_window.window);
			}
		}

        // transfer states
		if (!memory_window.goto_tooltip_active) {
			if (platform_state.enter && !last_platform_state.enter)
				emulation_mode = EM_RUN;
			else if (platform_state.space && !last_platform_state.space)
				emulation_mode = EM_STEP_INSTRUCTION;
			else if (platform_state.f && !last_platform_state.f)
				emulation_mode = EM_STEP_FRAME;
		}

		// activate and update memory window
		update_memory_window();

		// emulate
        switch (emulation_mode) {
        case EM_RUN:              emulation_mode_run(&cpu);              break;
        case EM_STEP_INSTRUCTION: emulation_mode_step_instruction(&cpu); break;
        case EM_STEP_FRAME:       emulation_mode_step_frame(&cpu);       break;
        default:
            assert(0 && "Unknown emulation mode");
            break;
        }


		// render
		elapsed_time = get_ticks() - last_frame_time;
		if (elapsed_time >= MS_PER_FRAME) {
			// FIXME(shaw): if emulation_mode == EM_STEP_FRAME then
			// render_memory_window() will not get called until the user
			// presses input to advance the frame
			if (emulation_mode == EM_STEP_INSTRUCTION || frame_prepared) {
				io_render_prepare();
				io_render_sprites();

				render_debug_window(&frame_arena, &cpu);
				render_memory_window();

				io_render_present();

				if (emulation_mode == EM_RUN || emulation_mode == EM_STEP_FRAME)
					frame_prepared = false;

				// NOTE(shaw): since get_ticks is a uint64_t and only has
				// millisecond precision, the MS_PER_FRAME will be truncated to
				// 16ms so fps will actually be 62.5 instead of 60
				last_frame_time += MS_PER_FRAME;
			}
		}

		arena_set_pos(&frame_arena, pos);
    }

    /* just let OS clean it up
     * delete_cart();
     */

#ifdef DEBUG_LOG
    if (logfile)
        fclose(logfile);
#endif

    return 0;
}

void emulation_mode_run(cpu_t *cpu) {
	/* update */
	if (apu_request_frame()) {
		while (!ppu_frame_completed()) {
			if (cpu->op_cycles == 0 && ppu_nmi())  {
				cpu_nmi(cpu);
				ppu_clear_nmi();
			}
			cpu_tick(cpu);
			apu_tick();
			ppu_tick(); ppu_tick(); ppu_tick();
		}

		ppu_clear_frame_completed();
		apu_flush_sound_buffer();
		frame_prepared = true;

		if (debug_window.window) {
			update_pattern_tables(0, pattern_tables);
			update_palettes(palettes);
		}
	}
}

void emulation_mode_step_instruction(cpu_t *cpu) {
	/* update */
	if (platform_state.space && !last_platform_state.space) {
		if (cpu->op_cycles == 0 && ppu_nmi()) {
			cpu_nmi(cpu);
			ppu_clear_nmi();
		}
		do {
			cpu_tick(cpu);
			apu_tick();
			ppu_tick(); ppu_tick(); ppu_tick();
		} while (cpu->op_cycles > 0);

		if (ppu_frame_completed()) {
			ppu_clear_frame_completed();
			apu_flush_sound_buffer();
		}

		if (debug_window.window) {
			update_pattern_tables(0, pattern_tables);
			update_palettes(palettes);
		}
	}
}

void emulation_mode_step_frame(cpu_t *cpu) {
	/* update */
	if (!frame_prepared && platform_state.f && !last_platform_state.f) {
		while (!ppu_frame_completed()) {
			if (cpu->op_cycles == 0 && ppu_nmi()) {
				cpu_nmi(cpu);
				ppu_clear_nmi();
			}
			cpu_tick(cpu);
			apu_tick();
			ppu_tick(); ppu_tick(); ppu_tick();
		}

		ppu_clear_frame_completed();
		apu_flush_sound_buffer();
		frame_prepared = true;

		if (debug_window.window) {
			update_pattern_tables(0, pattern_tables);
			update_palettes(palettes);
		}
	}
}

void init_debug_chr_viewer(sprite_t pattern_tables[2], sprite_t palettes[8]) {
    double pad = 0.0133333*DEBUG_WINDOW_WIDTH;
    double y_pos;

    /* pattern tables */
    double pat_width = 0.48*DEBUG_WINDOW_WIDTH;
    y_pos = DEBUG_WINDOW_HEIGHT - pat_width - pad;
    for (int i=0; i<2; ++i) {
        uint32_t *pixels = malloc(128*128*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        pattern_tables[i] = make_sprite(&debug_window, pixels, 
			128, 128, 
            (int)(pad + i*(pat_width+pad)), /* dest x */
            (int)y_pos,                                         /* dest y */
            (int)pat_width,                                     /* dest width */
            (int)pat_width);                                    /* dest height */
        register_sprite(&debug_window, &pattern_tables[i]);
    }

    /* palettes */
    double pal_width = 0.11*DEBUG_WINDOW_WIDTH;
    int pal_height = 10;
    y_pos -= 2*pad + pal_height;
    for (int i=0; i<8; ++i) {
        uint32_t *pixels = malloc(4*1*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        palettes[i] = make_sprite(&debug_window, pixels, 
			4, 1, 
            (int)(pad + i*(pal_width+pad)), /* dest x */
            (int)y_pos,                                         /* dest y */
            (int)pal_width,                                     /* dest width */
            (int)pal_height);                                   /* dest height */
        register_sprite(&debug_window, &palettes[i]);
    }
}

char **get_dasm_lines(Arena *arena, uint16_t pc) {
	int lines_above    = MAX_CODE_LINES/2 - (MAX_CODE_LINES%2 == 0);
	int lines_from_cur = MAX_CODE_LINES - lines_above;

	char **dasm_prev   = disassemble_n_cached_instructions(arena, lines_above);
	char **dasm_future = disassemble_n_instructions(arena, pc, lines_from_cur);

	char **dasm_lines = arena_alloc(arena, MAX_CODE_LINES * sizeof(char*));
	memcpy(dasm_lines, dasm_prev, lines_above * sizeof(char*));
	memcpy(dasm_lines+lines_above, dasm_future, lines_from_cur * sizeof(char*));

	return dasm_lines;
}

void render_cpu_state(cpu_t *cpu, char **lines) {
    snprintf(lines[0], MAX_DEBUG_LINE_CHARS+1, "N V - B D I Z C");
    snprintf(lines[1], MAX_DEBUG_LINE_CHARS+1, "%d %d %d %d %d %d %d %d",  
        (cpu->status >> 7) & 1, (cpu->status >> 6) & 1, (cpu->status >> 5) & 1, 
        (cpu->status >> 4) & 1, (cpu->status >> 3) & 1, (cpu->status >> 2) & 1, 
        (cpu->status >> 1) & 1, (cpu->status >> 0) & 1); 
    snprintf(lines[2], MAX_DEBUG_LINE_CHARS+1, "PC: %.4X", cpu->pc);
    snprintf(lines[3], MAX_DEBUG_LINE_CHARS+1, " A: %.2X", cpu->a);
    snprintf(lines[4], MAX_DEBUG_LINE_CHARS+1, " X: %.2X", cpu->x);
    snprintf(lines[5], MAX_DEBUG_LINE_CHARS+1, " Y: %.2X", cpu->y);
    snprintf(lines[6], MAX_DEBUG_LINE_CHARS+1, "SP: %.2X", cpu->sp);

    int pad = (int)(0.0133333 * DEBUG_WINDOW_WIDTH);
    for (int i=0; i<MAX_CPU_STATE_LINES; ++i) {
        char *line = lines[i];
        if (line == NULL) break;
        render_text(&debug_window, line, 
        	pad, 
        	(int)(i*FONT_CHAR_HEIGHT*FONT_SCALE + 3*pad));
    }
}   

void render_oam_info(void) {
    int i;
    uint8_t y,tile_id,attr,x;
    char line[MAX_DEBUG_LINE_CHARS+1];
    uint8_t *oam = ppu_get_oam();
    
    int pad = (int)(0.0133333 * DEBUG_WINDOW_WIDTH);
    for (i=0; i<MAX_CODE_LINES; ++i) {
        y       = oam[i*4];
        tile_id = oam[i*4+1];
        attr    = oam[i*4+2];
        x       = oam[i*4+3];
        
        snprintf(line, MAX_DEBUG_LINE_CHARS+1, "(%3d,%3d) %2X %2X", x,y,tile_id,attr);
        render_text(&debug_window, line, 
			pad,
            (int)(i*FONT_CHAR_HEIGHT*FONT_SCALE + 7*FONT_CHAR_HEIGHT*FONT_SCALE + pad));
    }
}

void render_code(uint16_t pc, char **lines, int num_lines) {
    int pad = (int)(0.0133333 * DEBUG_WINDOW_WIDTH);
	int current_ins = MAX_CODE_LINES/2 - (MAX_CODE_LINES%2 == 0);
	for (int i=0; i<num_lines; ++i) {
		render_text_color(&debug_window, lines[i],
			pad,
			(int)(i*FONT_CHAR_HEIGHT*FONT_SCALE + 7*FONT_CHAR_HEIGHT*FONT_SCALE + 4*pad),
			i == current_ins ? 0xFFA7ED : 0xFFFFFF);
	}
}

void render_debug_window(Arena *arena, cpu_t *cpu) {
	if (!debug_window.window) return;

	// disassemble
	char **dasm_lines = get_dasm_lines(arena, cpu->pc);

	render_cpu_state(cpu, cpu_state_lines);
	render_code(cpu->pc, dasm_lines, MAX_CODE_LINES);
}

#define MAX_LINE_LEN 74
void render_memory(void) {
	char buffer[MAX_LINE_LEN];
	int line_num = 0;
	int pad = 15;
	int num_visible_lines = (memory_window.height / TEXT_LINE_HEIGHT) - 1;
	uint16_t start = 16 * (uint16_t)(memory_window.scroll_y / TEXT_LINE_HEIGHT);
	start = MIN(start, 0xFFF0 - 16*num_visible_lines);
	uint16_t end = start + 16*num_visible_lines;

	for (uint16_t addr = start; addr >= start && addr <= end; addr += 16) {
		uint8_t bytes[16];
		char displays[17];
		for (int i=0; i<16; ++i) {
			bytes[i] = bus_read(addr+i);
			displays[i] = isprint(bytes[i]) ? bytes[i] : '.';
		}
		displays[16] = 0;

		snprintf(buffer, MAX_LINE_LEN, 
			"%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X |%s|",
			addr, bytes[0], bytes[1], bytes[2], bytes[3],
			bytes[4], bytes[5], bytes[6], bytes[7],
			bytes[8], bytes[9], bytes[10], bytes[11],
			bytes[12], bytes[13], bytes[14], bytes[15],
			displays);
			
		render_text(&memory_window, buffer, 
			pad,
            line_num*TEXT_LINE_HEIGHT + TEXT_LINE_HEIGHT);
		++line_num;
	}
}
#undef MAX_LINE_LEN

void render_memory_window(void) {
	if (!memory_window.window) return;
	render_memory();
	
	// TODO(shaw): goto address tooltip
	// if goto address tooltip is open, render it
		// draw a rectangle towards top of window
		// draw the input buffer inside the rectangle
		// ideally draw a cursor as well
	if (memory_window.goto_tooltip_active) {
		// draw a rectangle towards top of window
		int pad = 5;
		int w = 250;
		int h = TEXT_LINE_HEIGHT + 4*pad;
		int x = (int)(0.5*memory_window.width - 0.5*w);
		int y = 100;
		SDL_Rect goto_rect = { x, y, w, h };
		SDL_SetRenderDrawColor(memory_window.renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderFillRect(memory_window.renderer, &goto_rect);
		SDL_SetRenderDrawColor(memory_window.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderDrawRect(memory_window.renderer, &goto_rect);

		// draw the input buffer inside the rectangle
		int text_x = x + 2*pad;
		int text_y = y + TEXT_LINE_HEIGHT + (int)(1.5*pad);
		render_text(&memory_window, "goto addr: ", text_x, text_y);
		text_x += (int)(strlen("goto addr: ") * FONT_CHAR_WIDTH*FONT_SCALE);
		render_text(&memory_window, memory_window.goto_input_buf, text_x, text_y);
	}
}

#define SCROLL_MULTIPLIER 40
void update_memory_window(void) {
	window_state_t *w = &memory_window;
	// activate memory window
	if (platform_state.m && !last_platform_state.m) {
		if (!w->window) {
			io_init_window(w, "NES Memory", (int)MEMORY_WINDOW_WIDTH, (int)MEMORY_WINDOW_HEIGHT);

			// hardcoded amount that results in addr 0xFFF0 displaying at the bottom of the window
			// this assumes the font Consolas and font size 20
			w->max_scroll_y = 73080; 
		} else {
			SDL_ShowWindow(w->window);
			SDL_RaiseWindow(w->window);
		}
	}

	// bail if window hasn't been initialized or isn't in focus
	if (!w->window) return;
	if (!(SDL_GetWindowFlags(w->window) & SDL_WINDOW_INPUT_FOCUS)) return;

	// mousewheel scrolling
	if (platform_state.wheel_y) {
		w->scroll_y -= platform_state.wheel_y * SCROLL_MULTIPLIER;
		if (w->scroll_y < 0)
			w->scroll_y = 0;
		else if (w->scroll_y > w->max_scroll_y) 
			w->scroll_y = w->max_scroll_y;
		printf("%d\n", w->scroll_y);
	}

	// GOTO address
	// pop up a tooltip thing where you can type the address you want to jump to
	if (platform_state.g && !last_platform_state.g) {
		w->goto_tooltip_active = !w->goto_tooltip_active;
	}

	// if goto tooltip is open, record user input into some buffer
	if (w->goto_tooltip_active) {
		if (platform_state.hex_char && w->goto_input_index < 4) {
			w->goto_input_buf[w->goto_input_index++] = platform_state.hex_char;
		} else if (platform_state.backspace && !last_platform_state.backspace && w->goto_input_index > 0) {
			w->goto_input_buf[--(w->goto_input_index)] = 0;
		}

		// if enter is pressed, calculate the scroll offset needed to jump to the desired address and set scroll_y
		if (platform_state.enter && !last_platform_state.enter) {
			uint16_t addr = (uint16_t)strtol(w->goto_input_buf, NULL, 16);
			// align to 16 bytes to get the line the address is on
			addr &= 0xFFF0;
			printf("addr = 0x%04X\n", addr);

			int num_lines_above = addr / 16;
			w->scroll_y = num_lines_above * TEXT_LINE_HEIGHT;

			w->goto_input_index = 0;
			memset(w->goto_input_buf, 0, 5);
			w->goto_tooltip_active = false;
		}

		// ideally highlight the desired address 
	}


}
#undef SCROLL_MULTIPLIER
