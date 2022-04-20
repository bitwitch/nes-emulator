#include <stdio.h>
#include <stdlib.h>
#include "cpu_6502.h"
#include "bus.h"
#include "cart.h"

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
    /*io_init();*/

#ifdef DEBUG_LOG
    logfile = fopen("nestest.log", "w");
#endif

    for (;;) {
        /* get input */

        /* update */
        /*if (step_pressed)*/
        cpu_tick(&cpu);

        /* draw */

        /* draw nes */

        /* draw debug stuff */
            /* cpu state */
            /* cart vrom */
            /* some memory */
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
