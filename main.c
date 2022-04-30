#include <stdio.h>
#include <stdlib.h>
#include "cpu_6502.h"
#include "bus.h"
#include "cart.h"
#include "io.h"
#include "ppu.h"

#define MS_PER_FRAME (1000/60)

#ifdef DEBUG_LOG
extern FILE *logfile;
#endif

void init_debug_sidebar(sprite_t pattern_tables[2], sprite_t palettes[8], sprite_t *asm_quad);

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s ROM_FILE\n", argv[0]);
        exit(1);
    }

    cpu_t cpu;
    read_rom_file(argv[1]);
    io_init();

    /* LEAK: 
     * All of the malloc calls for sprite pixels are leaking. since the memory
     * is used for the lifetime of the application, we are just ignoring and
     * letting the operating system clean up at exit */

    uint32_t *pixels = malloc(NES_WIDTH*NES_HEIGHT*sizeof(uint32_t));
    if (!pixels) { perror("malloc"); exit(1); }
    sprite_t nes_quad = make_sprite(pixels, NES_WIDTH, NES_HEIGHT, 
        0, 0, NES_WIDTH*SCALE, NES_HEIGHT*SCALE);
    register_sprite(&nes_quad);

    /* initialize debug sidebar */
    sprite_t pattern_tables[2];
    sprite_t palettes[8];
    sprite_t asm_quad;
    init_debug_sidebar(pattern_tables, palettes, &asm_quad);

    /*char *asm_lines[11] = { "Well", "hello", "friends", ":^)" , NULL};*/
    
    /*render_text(asm_quad.pixels, asm_width, asm_height, asm_lines);*/

    /* TEMPORARY */
    /* TEMPORARY */
    for (int i=0; i<128*128; ++i) {
        pattern_tables[0].pixels[i] = 0xCAB192;
        pattern_tables[1].pixels[i] = 0x123040;
    }
    for (int i=0; i<8; ++i) {
        palettes[i].pixels[0] = 0xFF0000;
        palettes[i].pixels[1] = 0x00FF00;
        palettes[i].pixels[2] = 0x0000FF;
    }
    for (int i=0; i<asm_quad.w*asm_quad.h; ++i) {
        asm_quad.pixels[i] = 0xFFEEEE;
    }
    /* TEMPORARY */
    /* TEMPORARY */

    ppu_init(nes_quad.pixels);

    cpu_reset(&cpu);


#ifdef DEBUG_LOG
    logfile = fopen("nestest.log", "w");
#endif

    bool frame_prepared = false;
    uint64_t elapsed_time = 0;
    uint64_t last_frame_time = get_ticks();

    for (;;) {
        do_input();

        /* update */
        if (!frame_prepared) {
            while (!ppu_frame_completed()) {
                cpu_tick(&cpu);
                /*if (cpu.pc == 0x0001) break; [> DELETE ME!!!! <]*/
                ppu_tick(); ppu_tick(); ppu_tick();
            }
            ppu_clear_frame_completed();
            frame_prepared = true;

            update_pattern_tables(pattern_tables);
            update_palettes(palettes);
        }

        /* render */
        elapsed_time = get_ticks() - last_frame_time;
        if ((elapsed_time >= MS_PER_FRAME) && frame_prepared) {
            draw();
            /* draw debug stuff */
                /* cpu state */
                /* cart vrom */
                /* some memory */
            frame_prepared = false;
            last_frame_time += MS_PER_FRAME;

            /* NOTE(shaw): since get_ticks is a uint64_t and only has
             * millisecond precision, the MS_PER_FRAME will be truncated to
             * 16ms so fps will actually be 62.5 instead of 60 */
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
                        sprite_t *asm_quad)
{
    uint32_t *pixels; 
    double debug_width = NES_WIDTH*SCALE*(1-NES_DEBUG_RATIO)/(NES_DEBUG_RATIO);
    double pad = 0.0133333*debug_width;

    /* pattern tables */
    double pat_width = 0.48*debug_width;
    double y_pos = (NES_HEIGHT*SCALE) - pat_width - pad;
    for (int i=0; i<2; ++i) {
        pixels = malloc(128*128*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        pattern_tables[i] = make_sprite(pixels, 128, 128, 
            (NES_WIDTH*SCALE) + pad + i*(pat_width+pad),      /* dest x */
            y_pos,                                            /* dest y */
            pat_width,                                        /* dest width */
            pat_width);                                       /* dest height */
        register_sprite(&pattern_tables[i]);
    }

    /* palettes */
    double pal_width = 0.11*debug_width;
    int pal_height = 10;
    y_pos -= 2*pad + pal_height;
    for (int i=0; i<8; ++i) {
        pixels = malloc(4*1*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        palettes[i] = make_sprite(pixels, 4, 1, 
            (NES_WIDTH*SCALE) + pad + i*(pal_width+pad),      /* dest x */
            y_pos,                                            /* dest y */
            pal_width,                                        /* dest width */
            pal_height);                                      /* dest height */
        register_sprite(&palettes[i]);
    }

    int asm_width = (NES_WIDTH*(1-NES_DEBUG_RATIO)/NES_DEBUG_RATIO) - (2*pad/SCALE);
    int asm_height = (y_pos - 3*pad)/SCALE;
    pixels = malloc(asm_width*asm_height*sizeof(uint32_t));
    *asm_quad = make_sprite(pixels, asm_width, asm_height, 
        NES_WIDTH*SCALE + pad, 
        pad, 
        asm_width*SCALE, 
        asm_height*SCALE); 
    register_sprite(asm_quad);
}


