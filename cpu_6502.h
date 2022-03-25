#ifndef CPU_6502_H
#define CPU_6502_H

#include <stdint.h>

typedef struct {
    uint8_t a, x, y, sp, status;
    uint16_t pc;
    uint16_t initial_pc;
    int cycle_counter, interrupt_period;
} cpu_6502_t;

typedef enum {
    STATUS_C = 1 << 0;
    STATUS_Z = 1 << 1;
    STATUS_I = 1 << 2;
    STATUS_D = 1 << 3;
    STATUS_B = 1 << 4;
    STATUS_V = 1 << 6;
    STATUS_N = 1 << 7;
} status_mask_t;

int run_6502(cpu_6502_t *cpu);

uint8_t get_flag(cpu_6502_t *cpu, status_mask_t mask);

/*****************************************************************/
/* These need to be defined by the system incorporating the 6502 */
extern uint8_t read_address(uint16_t addr);
extern void write_address(uint16_t addr);
/*****************************************************************/

#endif 
