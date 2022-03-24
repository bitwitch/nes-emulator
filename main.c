#include <stdio.h>
#include "cpu_6502.h"

/*void init_memory(void);*/

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    cpu_6502_t cpu = {0};
    cpu.status = 1 << 5;
    cpu.interrupt_period = 1;

    run_6502(&cpu);

    return 0;
}
