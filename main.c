#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include "cpu_6502.h"
#include "bus.h"
#include "cart.h"
#include "io.h"
#include "ppu.h"

#define MS_PER_FRAME (1000/60)

#define MAX_CPU_STATE_LINES 36
#define MAX_DEBUG_LINE_CHARS 34
#define MAX_CODE_LINES 12

typedef enum {
    EM_RUN,
    EM_STEP_INSTRUCTION,
    EM_STEP_FRAME,
} emulation_mode_t;

#ifdef DEBUG_LOG
extern FILE *logfile;
#endif

static char *cpu_state_lines[MAX_CPU_STATE_LINES];

void init_debug_sidebar(sprite_t pattern_tables[2], sprite_t palettes[8], sprite_t *code_quad);
void render_cpu_state(cpu_t *cpu, char **cpu_state_lines);
void render_code(uint16_t addr, dasm_map_t *dasm);


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s ROM_FILE\n", argv[0]);
        exit(1);
    }

    cpu_t cpu;
    read_rom_file(argv[1]);
    io_init();
    platform_state_t platform_state = {0};
    platform_state_t last_platform_state = {0};

    /* LEAK: disassemble allocates memory for strings */
    dasm_map_t *dasm = disassemble(0xC000, 0xFFFF);

    /* LEAK: 
     * All of the malloc calls for sprite pixels are leaking. since the memory
     * is used for the lifetime of the application, we are just ignoring and
     * letting the operating system clean up at exit */

    for (int i=0; i<MAX_CPU_STATE_LINES-1; ++i) {
        cpu_state_lines[i] = calloc(MAX_DEBUG_LINE_CHARS+1, 1);
        if (!cpu_state_lines[i]) { perror("malloc"); exit(1); }
    }
    cpu_state_lines[MAX_CPU_STATE_LINES-1] = NULL;

    uint32_t *pixels = malloc(NES_WIDTH*NES_HEIGHT*sizeof(uint32_t));
    if (!pixels) { perror("malloc"); exit(1); }
    sprite_t nes_quad = make_sprite(pixels, NES_WIDTH, NES_HEIGHT, 
        0, 0, NES_WIDTH*SCALE, NES_HEIGHT*SCALE);
    register_sprite(&nes_quad);

    /* initialize debug sidebar */
    sprite_t pattern_tables[2];
    sprite_t palettes[8];
    sprite_t code_quad;
    init_debug_sidebar(pattern_tables, palettes, &code_quad);

    ppu_init(nes_quad.pixels);

    cpu_reset(&cpu);

#ifdef DEBUG_LOG
    logfile = fopen("nestest.log", "w");
#endif

    bool frame_prepared = false;
    uint64_t elapsed_time = 0;
    uint64_t last_frame_time = get_ticks();
    emulation_mode_t emulation_mode = EM_STEP_INSTRUCTION;

    for (;;) {
        last_platform_state = platform_state;
        do_input(&platform_state);

        switch (emulation_mode) {
        case EM_RUN:
        {
            /* update */
            if (!frame_prepared) {
                while (!ppu_frame_completed()) {
                    cpu_tick(&cpu);
                    /*if (cpu.cycles > 190000) exit(0);*/
                    /*if (cpu.pc == 0x0001) break; [> DELETE ME!!!! <]*/
                    ppu_tick(); ppu_tick(); ppu_tick();
                }
                ppu_clear_frame_completed();
                frame_prepared = true;

                /*update_pattern_tables(0, pattern_tables);*/
                /*update_palettes(palettes);*/
            }

            /* render */
            elapsed_time = get_ticks() - last_frame_time;
            if ((elapsed_time >= MS_PER_FRAME) && frame_prepared) {
                prepare_drawing();
                render_sprites();
                render_cpu_state(&cpu, cpu_state_lines);
                render_code(cpu.pc, dasm);
                draw();

                frame_prepared = false;
                last_frame_time += MS_PER_FRAME;

                /* NOTE(shaw): since get_ticks is a uint64_t and only has
                 * millisecond precision, the MS_PER_FRAME will be truncated to
                 * 16ms so fps will actually be 62.5 instead of 60 */
            }

            /* state transfer */
            if (platform_state.space && !last_platform_state.space)
                emulation_mode = EM_STEP_INSTRUCTION;
            else if (platform_state.f && !last_platform_state.f)
                emulation_mode = EM_STEP_FRAME;

            break;
        }

        case EM_STEP_INSTRUCTION:
        {
            /* update */
            if (platform_state.space && !last_platform_state.space) {
                do {
                    cpu_tick(&cpu);
                    /*if (cpu.cycles > 190000) exit(0);*/
                    /*if (cpu.pc == 0x0001) break; [> DELETE ME!!!! <]*/
                    ppu_tick(); ppu_tick(); ppu_tick();
                } while (cpu.op_cycles > 0);

                if (ppu_frame_completed()) {
                    ppu_clear_frame_completed();
                }
                update_pattern_tables(0, pattern_tables);
                update_palettes(palettes);
            }

            /* render */
            elapsed_time = get_ticks() - last_frame_time;
            if (elapsed_time >= MS_PER_FRAME) {
                prepare_drawing();
                render_sprites();
                render_cpu_state(&cpu, cpu_state_lines);
                render_code(cpu.pc, dasm);
                draw();
                last_frame_time += MS_PER_FRAME;
                /* NOTE(shaw): since get_ticks is a uint64_t and only has
                 * millisecond precision, the MS_PER_FRAME will be truncated to
                 * 16ms so fps will actually be 62.5 instead of 60 */
            }

            /* state transfer */
            if (platform_state.enter && !last_platform_state.enter)
                emulation_mode = EM_RUN;
            else if (platform_state.f && !last_platform_state.f)
                emulation_mode = EM_STEP_FRAME;
 
            break;
        }

        case EM_STEP_FRAME:
        {

            /* state transfer */
            if (platform_state.enter && !last_platform_state.enter)
                emulation_mode = EM_RUN;
            else if (platform_state.space && !last_platform_state.space)
                emulation_mode = EM_STEP_INSTRUCTION;

            break;
        }

        default:
            assert(0 && "Unknown emulation mode");
            break;
        }

    }

    /* just let OS clean it up
     * delete_cart(&cart);
     */

#ifdef DEBUG_LOG
    if (logfile)
        fclose(logfile);
#endif

    return 0;
}

void init_debug_sidebar(sprite_t pattern_tables[2], 
                        sprite_t palettes[8],
                        sprite_t *code_quad)
{

    double debug_width = NES_WIDTH*SCALE*(1-NES_DEBUG_RATIO)/(NES_DEBUG_RATIO);
    double pad = 0.0133333*debug_width;

    /* pattern tables */
    double pat_width = 0.48*debug_width;
    double y_pos = (NES_HEIGHT*SCALE) - pat_width - pad;
    for (int i=0; i<2; ++i) {
        uint32_t *pixels = malloc(128*128*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        pattern_tables[i] = make_sprite(pixels, 128, 128, 
            (NES_WIDTH*SCALE) + pad + i*(pat_width+pad),      /* dest x */
            y_pos,                                            /* dest y */
            pat_width,                                        /* dest width */
            pat_width);                                       /* dest height */
        register_sprite(&pattern_tables[i]);
    }
    update_pattern_tables(0, pattern_tables);

    /* palettes */
    double pal_width = 0.11*debug_width;
    int pal_height = 10;
    y_pos -= 2*pad + pal_height;
    for (int i=0; i<8; ++i) {
        uint32_t *pixels = malloc(4*1*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        palettes[i] = make_sprite(pixels, 4, 1, 
            (NES_WIDTH*SCALE) + pad + i*(pal_width+pad),      /* dest x */
            y_pos,                                            /* dest y */
            pal_width,                                        /* dest width */
            pal_height);                                      /* dest height */
        register_sprite(&palettes[i]);
    }
    update_palettes(palettes);

    /* disassembled code */
    (void)code_quad; 
    /*int asm_width = 2*((NES_WIDTH*(1-NES_DEBUG_RATIO)/NES_DEBUG_RATIO) - (2*pad/SCALE));*/
    /*int asm_height = 2*(y_pos - 3*pad)/SCALE;*/
    /*pixels = malloc(asm_width*asm_height*sizeof(uint32_t));*/
    /*if (!pixels) { perror("malloc"); exit(1); }*/
    /**code_quad = make_sprite(pixels, asm_width, asm_height, */
        /*NES_WIDTH*SCALE + pad, */
        /*pad, */
        /*0.5*asm_width*SCALE, */
        /*0.5*asm_height*SCALE); */
    /*register_sprite(code_quad);*/


    /* TEMPORARY */
    /* TEMPORARY */
    /*for (int i=0; i<128*128; ++i) {*/
        /*pattern_tables[0].pixels[i] = 0xCAB192;*/
        /*pattern_tables[1].pixels[i] = 0x123040;*/
    /*}*/
    /*for (int i=0; i<8; ++i) {*/
        /*palettes[i].pixels[0] = 0xFF0000;*/
        /*palettes[i].pixels[1] = 0x00FF00;*/
        /*palettes[i].pixels[2] = 0x0000FF;*/
    /*}*/
    /*for (int i=0; i<code_quad.w*code_quad.h; ++i) {*/
        /*code_quad.pixels[i] = 0xFFEEEE;*/
    /*}*/
    /* TEMPORARY */
    /* TEMPORARY */
}

void render_cpu_state(cpu_t *cpu, char **lines) {
    snprintf(lines[0], MAX_DEBUG_LINE_CHARS+1, "N V _ B D I Z C");
    snprintf(lines[1], MAX_DEBUG_LINE_CHARS+1, "%d %d %d %d %d %d %d %d",  
        (cpu->status >> 7) & 1, (cpu->status >> 6) & 1, (cpu->status >> 5) & 1, 
        (cpu->status >> 4) & 1, (cpu->status >> 3) & 1, (cpu->status >> 2) & 1, 
        (cpu->status >> 1) & 1, (cpu->status >> 0) & 1); 
    snprintf(lines[2], MAX_DEBUG_LINE_CHARS+1, "PC: %.4X", cpu->pc);
    snprintf(lines[3], MAX_DEBUG_LINE_CHARS+1, " A: %.2X", cpu->a);
    snprintf(lines[4], MAX_DEBUG_LINE_CHARS+1, " X: %.2X", cpu->x);
    snprintf(lines[5], MAX_DEBUG_LINE_CHARS+1, " Y: %.2X", cpu->y);
    snprintf(lines[6], MAX_DEBUG_LINE_CHARS+1, "SP: %.2X", cpu->sp);

    #define PAD (0.0133333*NES_WIDTH*SCALE*(1-NES_DEBUG_RATIO)/(NES_DEBUG_RATIO))
    for (int i=0; i<MAX_CPU_STATE_LINES; ++i) {
        char *line = lines[i];
        if (line == NULL) break;
        render_text(line, NES_WIDTH*SCALE+PAD, i*FONT_CHAR_HEIGHT*SCALE+PAD);
    }
    #undef PAD
}   

void render_code(uint16_t addr, dasm_map_t *dasm) {
    int ins_index = hmgeti(dasm, addr);
    int max_index = ins_index + MAX_CODE_LINES/2;
    if (max_index >= hmlen(dasm)) max_index = hmlen(dasm)-1;
    int min_index = max_index - (MAX_CODE_LINES-1); 
    if (min_index < 0) min_index = 0;

    #define PAD (0.0133333*NES_WIDTH*SCALE*(1-NES_DEBUG_RATIO)/(NES_DEBUG_RATIO))
    for (int i=min_index; i<=max_index; ++i) {
        if (i == ins_index)
            render_text_color(dasm[i].value, 
                NES_WIDTH*SCALE+PAD, 
                (i-min_index)*FONT_CHAR_HEIGHT*SCALE + 7*FONT_CHAR_HEIGHT*SCALE + PAD,
                0xFFA7ED);
        else
            render_text(dasm[i].value, 
                NES_WIDTH*SCALE+PAD, 
                (i-min_index)*FONT_CHAR_HEIGHT*SCALE + 7*FONT_CHAR_HEIGHT*SCALE + PAD);
    }
    #undef PAD
}

