#include <stdio.h>
#include <stdlib.h>
#include "cpu_6502.h"
#include "bus.h"
#include "cart.h"
#include "io.h"
#include "ppu.h"

#include <SDL2/SDL.h>

#ifdef DEBUG_LOG
extern FILE *logfile;
#endif

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s ROM_FILE\n", argv[0]);
        exit(1);
    }

    cpu_t cpu;
    read_rom_file(argv[1]);
    cpu_reset(&cpu);
    uint32_t *pixels = io_init();
    ppu_init(pixels);

#ifdef DEBUG_LOG
    logfile = fopen("nestest.log", "w");
#endif

    uint64_t start_time, frame_time;
    int fps = 0;

    for (;;) {
        start_time = SDL_GetTicks64();

        /* get input */

        /* update */
        cpu_tick(&cpu);
        if (cpu.pc == 0x0001) break;
        ppu_tick(); ppu_tick(); ppu_tick();

        /* draw */
        draw(fps);

        frame_time = SDL_GetTicks64() - start_time;
        fps = (frame_time > 0) ? 1000.0f / frame_time : 0.0f;

        /* draw debug stuff */
            /* cpu state */
            /* cart vrom */
            /* some memory */
    }

    /* just let OS clean it up
     * io_deinit();
     * delete_cart(&cart);
     */

#ifdef DEBUG_LOG
    if (logfile)
        fclose(logfile);
#endif

    return 0;
}
