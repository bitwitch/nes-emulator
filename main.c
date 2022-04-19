#include <stdio.h>
#include "cpu_6502.h"
#include "bus.h"
#include "cart.h"
#include "repl.h"

int main(int argc, char **argv) {

    /*if (argc < 2) {*/
        /*printf("Usage: %s ROM_FILE\n", argv[0]);*/
        /*exit(1);*/
    /*}*/

    /*cpu_t cpu;*/
    /*cart_t cart = ines_read(argv[1]);*/
    /*load_memory(address, cart.prg_rom, CART_PRG_ROM_SIZE(cart.header));*/
    /*delete_cart(&cart);*/
    /*cpu_reset(&cpu);*/

    (void)argc; (void)argv;
    return repl();

    /*init_memory();*/

    /*run_6502(&cpu);*/

    /*return 0;*/
}
