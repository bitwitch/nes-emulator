#ifndef CPU_6502_H
#define CPU_6502_H

#include <stdint.h>

#define OP_COUNT 256

typedef struct {
    uint8_t a, x, y, sp, status;
    uint16_t pc;
    uint16_t initial_pc;
    int cycle_counter, interrupt_period;
} cpu_6502_t;

/* These need to be defined by the components connected to the 6502 */
extern uint8_t read_address(uint16_t addr);
extern void write_address(uint16_t addr);


int run_6502(cpu_6502_t *cpu);

#endif 
