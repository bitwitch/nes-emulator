#include <assert.h>
#include <stdio.h>
#include "cpu_6502.h"
#include "ops.h"

typedef enum {
    IMP, 
    ABS, ABS_X, ABS_Y,
    IMM,
    IND, X_IND, IND_Y,
    REL,
    ZPG, ZPG_X, ZPG_Y
} addr_mode_t;

typedef struct {
    char name[4];
    uint8_t cycles;
    /* function pointer to address mode */
    uint16_t (*addr_mode)(cpu_6502_t *cpu);             
    /* function pointer to operation */
    void (*execute)(cpu_6502_t *cpu, uint16_t operand); 
} op_t;

typedef union {
    uint16_t w;
#ifdef CPU6502_BIG_ENDIAN
    struct { uint8_t h,l; } byte;
#else
    struct { uint8_t l,h; } byte;
#endif
} word_t;

#define OP_COUNT 256

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

static inline op_brk(uint16_t operand) {}
static inline op_ora(uint16_t operand) {}
static inline op_xxx(uint16_t operand) {}
static inline op_asl(uint16_t operand) {}

op_t ops[OP_COUNT] = {
    {"BRK", 7, &am_imp, &op_brk}, {"ORA", 6, &am_xind, &op_ora}, {"xxx", 2, &am_imp, &op_xxx}, {"xxx", 2, &am_imp, &op_xxx}, {"xxx", 2, &am_imp, &op_xxx}, {"ORA", 3, &am_zpg, &op_ora}, {"ASL", 5, &am_zpg, &op_asl}, {"xxx", 2, &am_imp, &op_xxx}, {"PHP", 3, &am_imp, &op_php}, {"ORA", 2, &am_imm, &op_ora}, {"ASL", 2, &am_imp, &op_asl}, {"xxx", 2, &am_imp, &op_xxx}, {"xxx", 2, &am_imp, &op_xxx}, {"ORA", 4, &am_abs, &op_ora}, {"ASL", 6, &am_abs, &op_asl}, {"xxx", 2, &am_imp, &op_xxx},



};



/* calculates the operand to an instruction, based on the addressing mode */
static uint16_t calculate_operand(cpu_6502_t *cpu, addr_mode_t addr_mode) {
    word_t operand = {0};
    switch (addr_mode) {
        case IMP: 
            return 0; /* no operand used */
        case ABS: 
            operand.byte.l = read_address(cpu->pc++);
            operand.byte.h = read_address(cpu->pc++);
            break;
        case ABS_X: 
            operand.byte.l = read_address(cpu->pc++);
            operand.byte.h = read_address(cpu->pc++);
            operand.w += cpu->x;
            break;
        case ABS_Y: 
            operand.byte.l = read_address(cpu->pc++);
            operand.byte.h = read_address(cpu->pc++);
            operand.w += cpu->y;
            break;
        case IMM: 
            operand.byte.l = read_address(cpu->pc++);
            break;
        case IND: 
        {
            word_t pointer = {0};
            pointer.byte.l = read_address(cpu->pc++);
            pointer.byte.h = read_address(cpu->pc++);
            operand.byte.l = read_address(pointer.w);
            operand.byte.h = read_address(pointer.w+1);
            break;
        }
        case X_IND: 
        {
            uint8_t zpg = cpu->x + read_address(cpu->pc++);
            operand.byte.l = read_address(zpg);
            operand.byte.h = read_address(zpg+1);
            /* TODO(shaw) must handle if the byte at zpg+1 is not in page zero */
            break;
        }
        case IND_Y: 
        {
            uint8_t zpg = read_address(cpu->pc++);
            uint16_t sum = cpu->y + read_address(zpg);
            uint8_t carry = 1 && (sum >> 8); 
            operand.byte.l = sum & 0xFF;
            operand.byte.h = read_address(zpg+1) + carry;
            break;
        }
        case REL: 
        {
            /* NOTE: signed offset */
            int8_t offset = read_address(cpu->pc++);
            operand.w = cpu->pc + offset;
            /* TODO(shaw) if a page transition occurs, then an extra cycle must
             * be added to execution */
            break;
        }
        case ZPG: 
            operand.byte.l = read_address(cpu->pc++);
            break;
        case ZPG_X: 
            operand.byte.l = cpu->x + read_address(cpu->pc++);
            break;
        case ZPG_Y: 
            operand.byte.l = cpu->y + read_address(cpu->pc++);
            break;
        default: 
            assert(0 && "unknown addressing mode");
            break;
    }
    return operand.w;
}

static void execute_op(cpu_6502_t *cpu, op_t op, uint16_t operand) {
    
}

/* TODO: TEMPORARY */
/* TODO: TEMPORARY */
/* TODO: TEMPORARY */
int exit_required = 0;

int run_6502(cpu_6502_t *cpu) {
    cpu->cycle_counter = cpu->interrupt_period;
    cpu->pc = cpu->initial_pc;

    uint8_t opcode;
    uint16_t address;
    op_t op;

    for(;;)
    {
        opcode = read_address(cpu->pc++);
        op = ops[opcode];
        operand = (*op.addr_mode)(&cpu);
        (*op.execute)(&cpu, operand);

        /* TEMPORARY */
        if (cpu->pc >= 256) exit_required = 1;
        /* TEMPORARY */
        
        cpu->cycle_counter -= op.cycles;

        if (cpu->cycle_counter <= 0) {
            /* Check for interrupts and do other */
            /* cyclic tasks here                 */

            cpu->cycle_counter += cpu->interrupt_period;
            if (exit_required) break;
        }
    }

    return 0;

}
