#include <assert.h>
#include <stdio.h>
#include "cpu_6502.h"
#include "ops.h"

typedef union {
    uint16_t w;
#ifdef CPU6502_BIG_ENDIAN
    struct { uint8_t h,l; } byte;
#else
    struct { uint8_t l,h; } byte;
#endif
} word_t;


uint8_t get_flag(cpu_6502_t *cpu, status_mask_t mask) {
    return (cpu->status & mask) != 0;
}

/****************************************************************************/
/* address modes */
/****************************************************************************/
uint16_t am_imp(cpu_6502_t *cpu) { return 0; }

uint16_t am_abs(cpu_6502_t *cpu) {
    word_t operand;
    operand.byte.l = read_address(cpu->pc++);
    operand.byte.h = read_address(cpu->pc++);
    return operand.w;
}

uint16_t am_abs_x(cpu_6502_t *cpu) {
    word_t operand;
    operand.byte.l = read_address(cpu->pc++);
    operand.byte.h = read_address(cpu->pc++);
    operand.w += cpu->x;
    return operand.w;
}

uint16_t am_abs_y(cpu_6502_t *cpu) {
    word_t operand;
    operand.byte.l = read_address(cpu->pc++);
    operand.byte.h = read_address(cpu->pc++);
    operand.w += cpu->y;
    return operand.w;
}

uint16_t am_imm(cpu_6502_t *cpu) {
    return read_address(cpu->pc++);
}

uint16_t am_ind(cpu_6502_t *cpu) {
    word_t operand, pointer;
    pointer.byte.l = read_address(cpu->pc++);
    pointer.byte.h = read_address(cpu->pc++);
    operand.byte.l = read_address(pointer.w);
    operand.byte.h = read_address(pointer.w+1);
    return operand.w;
}

uint16_t am_x_ind(cpu_6502_t *cpu) {
    word_t operand;
    uint8_t zpg = cpu->x + read_address(cpu->pc++);
    operand.byte.l = read_address(zpg);
    operand.byte.h = read_address(zpg+1);
    /* TODO(shaw) must handle if the byte at zpg+1 is not in page zero */
    return operand.w;
}

uint16_t am_ind_y(cpu_6502_t *cpu) {
    word_t operand;
    uint8_t zpg = read_address(cpu->pc++);
    uint16_t sum = cpu->y + read_address(zpg);
    uint8_t carry = (sum >> 8) != 0; 
    operand.byte.l = sum & 0xFF;
    operand.byte.h = read_address(zpg+1) + carry;
    return operand.w;
}

uint16_t am_rel(cpu_6502_t *cpu) {
    /* NOTE: signed offset */
    int8_t offset = read_address(cpu->pc++);
    return cpu->pc + offset;
    /* TODO(shaw) if a page transition occurs, then an extra cycle must
     * be added to execution */
}

uint16_t am_zpg(cpu_6502_t *cpu) {
    return read_address(cpu->pc++);
}

uint16_t am_zpg_x(cpu_6502_t *cpu) {
    uint8_t operand = cpu->x + read_address(cpu->pc++);
    return operand;
}

uint16_t am_zpg_y(cpu_6502_t *cpu) {
    uint8_t operand = cpu->y + read_address(cpu->pc++);
    return operand;
}


/****************************************************************************/
/* operations */
/****************************************************************************/
uint8_t op_adc(cpu_6502_t *cpu, uint8_t opcode, uint16_t operand) {
    uint16_t temp = cpu->a + operand + get_flag(cpu, STATUS_C);
}

uint8_t op_and(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_asl(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bcc(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bcs(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_beq(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bit(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bmi(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bne(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bpl(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_brk(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bvc(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bvs(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_clc(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cld(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cli(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_clv(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cmp(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cpx(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cpy(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_dec(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_dex(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_dey(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_eor(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_inc(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_inx(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_iny(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_jmp(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_jsr(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_lda(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_ldx(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_ldy(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_lsr(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_nop(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_ora(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_pha(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_php(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_pla(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_plp(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_rol(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_ror(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_rti(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_rts(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_sbc(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_sec(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_sed(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_sei(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_sta(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_stx(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_sty(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_tax(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_tay(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_tsx(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_txa(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_txs(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_tya(cpu_6502_t *cpu, uint16_t operand){
    assert(0 && "not implemented");
    return 0;
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
