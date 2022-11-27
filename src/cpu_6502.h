#ifndef CPU_6502_H
#define CPU_6502_H

#include <stdint.h>
#include <stdbool.h>

#define OP_COUNT 256

typedef struct cpu_t cpu_t;

struct cpu_t {
    uint8_t a, x, y, sp, status;
    uint16_t pc;
    uint8_t opcode;   /* stores current opcode fetched, used for the few instructions that have memory and accumulator modes */
    uint8_t op_cycles;
    uint64_t cycles;
    bool running;
};

typedef struct {
    char name[4];
    uint8_t cycles;
    /* function pointer to operation */
    uint8_t (*execute)(cpu_t *cpu, uint16_t addr);
    /* function pointer to address mode */
    uint8_t (*addr_mode)(cpu_t *cpu, uint16_t *addr);
} op_t;

typedef enum {
    STATUS_C = 1 << 0,
    STATUS_Z = 1 << 1,
    STATUS_I = 1 << 2,
    STATUS_D = 1 << 3,

    /* bits 4 and 5 dont actually exist in the status register
     * see https://www.nesdev.org/wiki/Status_flags#The_B_flag */
    STATUS_B = 1 << 4,
    STATUS_U = 1 << 5,

    STATUS_V = 1 << 6,
    STATUS_N = 1 << 7
} status_mask_t;

typedef struct {
    uint16_t key;    /* address */
    char *value;     /* instruction string */
} dasm_map_t;

void cpu_irq(cpu_t *cpu);
void cpu_nmi(cpu_t *cpu);

void cpu_tick(cpu_t *cpu);  /* executes a single instruction */
int cpu_run(cpu_t *cpu);
void cpu_reset(cpu_t *cpu);

dasm_map_t *disassemble(uint16_t start, uint16_t stop);

/* exposed so the repl can easily inspect cpu internal state */
uint8_t get_flag(cpu_t *cpu, status_mask_t flag);
void set_flag(cpu_t *cpu, status_mask_t flag, bool value);
void debug_log_instruction(cpu_t *cpu);

#endif 
