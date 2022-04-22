#include <stdio.h>
#include <stdlib.h>
#include "cpu_6502.h"
#include "bus.h"
#include "cart.h"
#include "io.h"
#include "ppu.h"

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

#ifdef DEBUG_LOG
    logfile = fopen("nestest.log", "w");
#endif

    for (;;) {
        /* get input */

        /* update */
        cpu_tick(&cpu);
        /*ppu_tick(); ppu_tick(); ppu_tick();*/

        for (int i=0; i<WIDTH*HEIGHT; ++i) {
            uint8_t val = rand() % 256;
            pixels[i] = (val << 16) | (val << 8) | val;
        }

        /* draw */
        draw();

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
