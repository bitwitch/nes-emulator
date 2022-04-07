#ifndef CPU_6502_H
#define CPU_6502_H

#include <stdint.h>

#define OP_COUNT 256

typedef struct {
    uint8_t a, x, y, sp, status;
    uint16_t pc;
    int cycle_counter, interrupt_period;
    int running;
} cpu_t;

typedef struct {
    char name[4];
    uint8_t cycles;
    /* function pointer to operation */
    uint8_t (*execute)(cpu_t *cpu, uint16_t addr);
    /* function pointer to address mode */
    uint16_t (*addr_mode)(cpu_t *cpu);
} op_t;

typedef enum {
    STATUS_C = 0,
    STATUS_Z = 1,
    STATUS_I = 2,
    STATUS_D = 3,
    STATUS_B = 4,
    STATUS_V = 6,
    STATUS_N = 7
} status_bit_t;


void cpu_tick(cpu_t *cpu);  /* executes a single instruction */
int cpu_run(cpu_t *cpu);
void cpu_reset(cpu_t *cpu);

/* exposed so the repl can easily inspect cpu internal state */
uint8_t get_flag(cpu_t *cpu, status_bit_t sbit);
void set_flag(cpu_t *cpu, status_bit_t sbit, int value);


/*****************************************************************/
/* These need to be defined by the system incorporating the 6502 */
extern uint8_t bus_read(uint16_t addr);
extern void bus_write(uint16_t addr, uint8_t data);
/*****************************************************************/

#endif 
