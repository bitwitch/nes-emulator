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
void render_cpu_state(cpu_t *cpu, char **cpu_state_lines);
void render_code(uint16_t pc, char **lines, int num_lines);
void render_oam_info(void);
void render_memory(void);
void emulation_mode_run(cpu_t *cpu);
void emulation_mode_step_instruction(cpu_t *cpu);
void emulation_mode_step_frame(cpu_t *cpu);

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

	Arena dasm_arena = {0};
	arena_grow(&dasm_arena, ARENA_BLOCK_SIZE); // initialize so that we can call arena_get_pos()
	init_cached_ins_addrs(&cpu);

    frame_prepared = false;
    elapsed_time = 0;
    last_frame_time = get_ticks();
    emulation_mode_t emulation_mode = EM_RUN;

    for (;;) {
        last_platform_state = platform_state;
        do_input();

	
		// activate debug window
		if (platform_state.tilde) {
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

		// activate memory window
		if (platform_state.m) {
			if (!memory_window.window) {
				io_init_window(&memory_window, "NES Memory", (int)MEMORY_WINDOW_WIDTH, (int)MEMORY_WINDOW_HEIGHT);
			} else {
				SDL_ShowWindow(memory_window.window);
				SDL_RaiseWindow(memory_window.window);
			}
		}

        // transfer states
        if (platform_state.enter && !last_platform_state.enter)
            emulation_mode = EM_RUN;
        else if (platform_state.space && !last_platform_state.space)
            emulation_mode = EM_STEP_INSTRUCTION;
        else if (platform_state.f && !last_platform_state.f)
            emulation_mode = EM_STEP_FRAME;

		// emulate
        switch (emulation_mode) {
        case EM_RUN:              emulation_mode_run(&cpu);              break;
        case EM_STEP_INSTRUCTION: emulation_mode_step_instruction(&cpu); break;
        case EM_STEP_FRAME:       emulation_mode_step_frame(&cpu);       break;
        default:
            assert(0 && "Unknown emulation mode");
            break;
        }

		// disassemble
		char *pos = arena_get_pos(&dasm_arena);
		char **dasm_lines = get_dasm_lines(&dasm_arena, cpu.pc);

		// render
		elapsed_time = get_ticks() - last_frame_time;
		if (elapsed_time >= MS_PER_FRAME) {
			if (emulation_mode == EM_STEP_INSTRUCTION || frame_prepared) {
				io_render_prepare();

				io_render_sprites();
				render_cpu_state(&cpu, cpu_state_lines);
				render_code(cpu.pc, dasm_lines, MAX_CODE_LINES);
				render_memory();

				io_render_present();

				if (emulation_mode == EM_RUN || emulation_mode == EM_STEP_FRAME)
					frame_prepared = false;

				// NOTE(shaw): since get_ticks is a uint64_t and only has
				// millisecond precision, the MS_PER_FRAME will be truncated to
				// 16ms so fps will actually be 62.5 instead of 60
				last_frame_time += MS_PER_FRAME;
			}
		}

		arena_set_pos(&dasm_arena, pos);
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


#define MAX_LINE_LEN 64
void render_memory(void) {
	char buffer[MAX_LINE_LEN];
	int line_num = 0;
	int pad = 15;
	for (uint16_t addr = 0; addr < 16*64; addr += 16) {
		snprintf(buffer, MAX_LINE_LEN, 
			"%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
			addr, bus_read(addr), bus_read(addr+1), bus_read(addr+2), bus_read(addr+3),
			bus_read(addr+4), bus_read(addr+5), bus_read(addr+6), bus_read(addr+7),
			bus_read(addr+8), bus_read(addr+9), bus_read(addr+10), bus_read(addr+11),
			bus_read(addr+12), bus_read(addr+13), bus_read(addr+14), bus_read(addr+15));
		render_text(&memory_window, buffer, 
			pad,
            (int)(line_num*FONT_CHAR_HEIGHT*FONT_SCALE + 2*pad));
		++line_num;
	}
}
#undef MAX_LINE_LEN

