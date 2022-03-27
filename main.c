#include <stdio.h>
#include "cpu_6502.h"
#include "repl.h"

void init_memory(void);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    return repl();

    /*init_memory();*/

    /*run_6502(&cpu);*/

    /*return 0;*/
}
