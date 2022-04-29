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
     * io_deinit();
     * delete_cart(&cart);
     */

#ifdef DEBUG_LOG
    if (logfile)
        fclose(logfile);
#endif

    return 0;
}


