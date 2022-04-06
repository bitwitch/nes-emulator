#ifndef CPU_6502_H
#define CPU_6502_H

#include <stdint.h>

typedef struct {
    uint8_t a, x, y, sp, status;
    uint16_t pc;
    int cycle_counter, interrupt_period;
} cpu_t;

typedef enum {
    STATUS_C = 0,
    STATUS_Z = 1,
    STATUS_I = 2,
    STATUS_D = 3,
    STATUS_B = 4,
    STATUS_V = 6,
    STATUS_N = 7
} status_bit_t;

int run_6502(cpu_t *cpu);
void reset_6502(cpu_t *cpu);

uint8_t get_flag(cpu_t *cpu, status_bit_t sbit);
void set_flag(cpu_t *cpu, status_bit_t sbit, int value);

/*****************************************************************/
/* These need to be defined by the system incorporating the 6502 */
extern uint8_t read_address(uint16_t addr);
extern void write_address(uint16_t addr, uint8_t data);
/*****************************************************************/

#endif 
