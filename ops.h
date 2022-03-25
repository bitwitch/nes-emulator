#ifndef _OPS_H
#define _OPS_H

#include <stdint.h>

#define OP_COUNT 256

typedef struct cpu_6502_t cpu_6502_t;

typedef struct {
    char name[4];
    uint8_t cycles;
    /* function pointer to address mode */
    uint16_t (*addr_mode)(cpu_6502_t *cpu);             
    /* function pointer to operation */
    void (*execute)(cpu_6502_t *cpu, uint16_t operand); 
} op_t;

/* address modes */
uint16_t am_imp(cpu_6502_t *cpu);
uint16_t am_abs(cpu_6502_t *cpu);
uint16_t am_abs_x(cpu_6502_t *cpu);
uint16_t am_abs_y(cpu_6502_t *cpu);
uint16_t am_imm(cpu_6502_t *cpu);
uint16_t am_ind(cpu_6502_t *cpu);
uint16_t am_x_ind(cpu_6502_t *cpu);
uint16_t am_ind_y(cpu_6502_t *cpu);
uint16_t am_rel(cpu_6502_t *cpu);
uint16_t am_zpg(cpu_6502_t *cpu);
uint16_t am_zpg_x(cpu_6502_t *cpu);
uint16_t am_zpg_y(cpu_6502_t *cpu);

/* operations */
uint8_t op_adc(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_and(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_asl(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bcc(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bcs(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_beq(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bit(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bmi(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bne(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bpl(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_brk(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bvc(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_bvs(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_clc(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_cld(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_cli(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_clv(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_cmp(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_cpx(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_cpy(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_dec(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_dex(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_dey(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_eor(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_inc(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_inx(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_iny(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_jmp(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_jsr(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_lda(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_ldx(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_ldy(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_lsr(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_nop(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_ora(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_pha(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_php(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_pla(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_plp(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_rol(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_ror(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_rti(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_rts(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_sbc(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_sec(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_sed(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_sei(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_sta(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_stx(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_sty(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_tax(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_tay(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_tsx(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_txa(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_txs(cpu_6502_t *cpu, uint16_t operand);
uint8_t op_tya(cpu_6502_t *cpu, uint16_t operand);

extern op_t ops[OP_COUNT];

#endif
