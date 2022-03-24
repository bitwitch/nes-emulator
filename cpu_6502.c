#include <assert.h>
#include <stdio.h>
#include "cpu_6502.h"

/* 
   cyles table taken from Marat Fayzullin
   https://fms.komkon.org/EMUL8/
*/
static uint8_t cycles[OP_COUNT] = {
  7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  2,5,2,5,4,4,4,4,2,4,2,5,4,4,4,4,
  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7
};


int exit_required = 0;

int run_6502(cpu_6502_t *cpu) {
    cpu->cycle_counter = cpu->interrupt_period;
    cpu->pc = cpu->initial_pc;

    uint8_t opcode;

    for(;;)
    {
        opcode = read_address(cpu->pc++);

        /* TEMPORARY */
        if (cpu->pc >= 256) exit_required = 1;
        /* TEMPORARY */
        

        switch (opcode) 
        {
#include "opcodes.h"
        }

        cpu->cycle_counter -= cycles[opcode];

        if (cpu->cycle_counter <= 0) {
            /* Check for interrupts and do other */
            /* cyclic tasks here                 */

            cpu->cycle_counter += cpu->interrupt_period;
            if (exit_required) break;
        }
    }

    return 0;

}
