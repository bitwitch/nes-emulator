#ifndef _OPS_H
#define _OPS_H

#include <stdint.h>
#include "cpu_6502.h"

#define OP_COUNT 256


typedef struct {
    char name[4];
    uint8_t cycles;
    /* function pointer to operation */
    uint8_t (*execute)(cpu_t *cpu, uint16_t addr);
    /* function pointer to address mode */
    uint16_t (*addr_mode)(cpu_t *cpu);
} op_t;

/* address modes */
uint16_t am_imp(cpu_t *cpu);
uint16_t am_abs(cpu_t *cpu);
uint16_t am_abs_x(cpu_t *cpu);
uint16_t am_abs_y(cpu_t *cpu);
uint16_t am_imm(cpu_t *cpu);
uint16_t am_ind(cpu_t *cpu);
uint16_t am_x_ind(cpu_t *cpu);
uint16_t am_ind_y(cpu_t *cpu);
uint16_t am_rel(cpu_t *cpu);
uint16_t am_zpg(cpu_t *cpu);
uint16_t am_zpg_x(cpu_t *cpu);
uint16_t am_zpg_y(cpu_t *cpu);

/* operations */
uint8_t op_adc(cpu_t *cpu, uint16_t addr);
uint8_t op_and(cpu_t *cpu, uint16_t addr);
uint8_t op_asl(cpu_t *cpu, uint16_t addr);
uint8_t op_bcc(cpu_t *cpu, uint16_t addr);
uint8_t op_bcs(cpu_t *cpu, uint16_t addr);
uint8_t op_beq(cpu_t *cpu, uint16_t addr);
uint8_t op_bit(cpu_t *cpu, uint16_t addr);
uint8_t op_bmi(cpu_t *cpu, uint16_t addr);
uint8_t op_bne(cpu_t *cpu, uint16_t addr);
uint8_t op_bpl(cpu_t *cpu, uint16_t addr);
uint8_t op_brk(cpu_t *cpu, uint16_t addr);
uint8_t op_bvc(cpu_t *cpu, uint16_t addr);
uint8_t op_bvs(cpu_t *cpu, uint16_t addr);
uint8_t op_clc(cpu_t *cpu, uint16_t addr);
uint8_t op_cld(cpu_t *cpu, uint16_t addr);
uint8_t op_cli(cpu_t *cpu, uint16_t addr);
uint8_t op_clv(cpu_t *cpu, uint16_t addr);
uint8_t op_cmp(cpu_t *cpu, uint16_t addr);
uint8_t op_cpx(cpu_t *cpu, uint16_t addr);
uint8_t op_cpy(cpu_t *cpu, uint16_t addr);
uint8_t op_dec(cpu_t *cpu, uint16_t addr);
uint8_t op_dex(cpu_t *cpu, uint16_t addr);
uint8_t op_dey(cpu_t *cpu, uint16_t addr);
uint8_t op_eor(cpu_t *cpu, uint16_t addr);
uint8_t op_inc(cpu_t *cpu, uint16_t addr);
uint8_t op_inx(cpu_t *cpu, uint16_t addr);
uint8_t op_iny(cpu_t *cpu, uint16_t addr);
uint8_t op_jmp(cpu_t *cpu, uint16_t addr);
uint8_t op_jsr(cpu_t *cpu, uint16_t addr);
uint8_t op_lda(cpu_t *cpu, uint16_t addr);
uint8_t op_ldx(cpu_t *cpu, uint16_t addr);
uint8_t op_ldy(cpu_t *cpu, uint16_t addr);
uint8_t op_lsr(cpu_t *cpu, uint16_t addr);
uint8_t op_nop(cpu_t *cpu, uint16_t addr);
uint8_t op_ora(cpu_t *cpu, uint16_t addr);
uint8_t op_pha(cpu_t *cpu, uint16_t addr);
uint8_t op_php(cpu_t *cpu, uint16_t addr);
uint8_t op_pla(cpu_t *cpu, uint16_t addr);
uint8_t op_plp(cpu_t *cpu, uint16_t addr);
uint8_t op_rol(cpu_t *cpu, uint16_t addr);
uint8_t op_ror(cpu_t *cpu, uint16_t addr);
uint8_t op_rti(cpu_t *cpu, uint16_t addr);
uint8_t op_rts(cpu_t *cpu, uint16_t addr);
uint8_t op_sbc(cpu_t *cpu, uint16_t addr);
uint8_t op_sec(cpu_t *cpu, uint16_t addr);
uint8_t op_sed(cpu_t *cpu, uint16_t addr);
uint8_t op_sei(cpu_t *cpu, uint16_t addr);
uint8_t op_sta(cpu_t *cpu, uint16_t addr);
uint8_t op_stx(cpu_t *cpu, uint16_t addr);
uint8_t op_sty(cpu_t *cpu, uint16_t addr);
uint8_t op_tax(cpu_t *cpu, uint16_t addr);
uint8_t op_tay(cpu_t *cpu, uint16_t addr);
uint8_t op_tsx(cpu_t *cpu, uint16_t addr);
uint8_t op_txa(cpu_t *cpu, uint16_t addr);
uint8_t op_txs(cpu_t *cpu, uint16_t addr);
uint8_t op_tya(cpu_t *cpu, uint16_t addr);

uint8_t op_xxx(cpu_t *cpu, uint16_t addr);

extern op_t ops[OP_COUNT];

#endif
