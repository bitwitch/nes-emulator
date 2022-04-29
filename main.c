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

sprite_t palettes[8];
sprite_t pattern_tables[2];

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s ROM_FILE\n", argv[0]);
        exit(1);
    }

    cpu_t cpu;
    read_rom_file(argv[1]);
    cpu_reset(&cpu);
    io_init();

    uint32_t *pixels; 
    /* LEAK: 
     * All of the malloc calls for sprite pixels are leaking. since the memory
     * is used for the lifetime of the application, we are just ignoring and
     * letting the operating system clean up at exit */

    pixels = malloc(NES_WIDTH*NES_HEIGHT*sizeof(uint32_t));
    if (!pixels) { perror("malloc"); exit(1); }
    sprite_t nes_quad = make_sprite(pixels, NES_WIDTH, NES_HEIGHT, 
        0, 0, NES_WIDTH*SCALE, NES_HEIGHT*SCALE);
    register_sprite(&nes_quad);

    /* pattern tables */
    for (int i=0; i<2; ++i) {
        pixels = malloc(128*128*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        pattern_tables[i] = make_sprite(pixels, 128, 128, i*(128+3), 0, 128, 128);
        register_sprite(&pattern_tables[i]);
    }

    /* palettes */
    for (int i=0; i<8; ++i) {
        pixels = malloc(4*1*sizeof(uint32_t));
        if (!pixels) { perror("malloc"); exit(1); }
        palettes[i] = make_sprite(pixels, 4, 1, i*(4+1)*10, 200, 4*10, 1*10);
        register_sprite(&palettes[i]);
    }
    /* TEMPORARY */
    for (int i=0; i<8; ++i) {
        palettes[i].pixels[0] = 0xFFFF0000;
        palettes[i].pixels[1] = 0xFF00FF00;
        palettes[i].pixels[2] = 0xFF0000FF;
    }

    ppu_init(nes_quad.pixels);

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
                /*cpu_tick(&cpu);*/
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


