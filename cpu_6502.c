#include <assert.h>
#include <stdio.h>
#include "cpu_6502.h"


typedef union {
    uint16_t w;
#ifdef CPU6502_BIG_ENDIAN
    struct { uint8_t h,l; } byte;
#else
    struct { uint8_t l,h; } byte;
#endif
} word_t;

uint8_t get_flag(cpu_t *cpu, status_bit_t sbit) {
    uint8_t mask = 1 << sbit;
    return (cpu->status & mask) != 0;
}

void set_flag(cpu_t *cpu, status_bit_t sbit, int value) {
    assert(value == 0 || value == 1);
    uint8_t mask = 1 << sbit;
    cpu->status = (cpu->status & (~mask)) | value << sbit;
}

/****************************************************************************/
/* address modes */
/****************************************************************************/
uint16_t am_imp(cpu_t *cpu) { 
    (void)cpu; 
    return 0; 
}

uint16_t am_abs(cpu_t *cpu) {
    word_t addr;
    addr.byte.l = bus_read(cpu->pc++);
    addr.byte.h = bus_read(cpu->pc++);
    return addr.w;
}

uint16_t am_abs_x(cpu_t *cpu) {
    word_t addr;
    addr.byte.l = bus_read(cpu->pc++);
    addr.byte.h = bus_read(cpu->pc++);
    addr.w += cpu->x;
    return addr.w;
}

uint16_t am_abs_y(cpu_t *cpu) {
    word_t addr;
    addr.byte.l = bus_read(cpu->pc++);
    addr.byte.h = bus_read(cpu->pc++);
    addr.w += cpu->y;
    return addr.w;
}

uint16_t am_imm(cpu_t *cpu) {
    return cpu->pc++;
}

uint16_t am_ind(cpu_t *cpu) {
    word_t addr, pointer;
    pointer.byte.l = bus_read(cpu->pc++);
    pointer.byte.h = bus_read(cpu->pc++);
    addr.byte.l = bus_read(pointer.w);
    addr.byte.h = bus_read(pointer.w+1);
    return addr.w;
}

uint16_t am_x_ind(cpu_t *cpu) {
    word_t addr;
    uint8_t zpg = cpu->x + bus_read(cpu->pc++);
    addr.byte.l = bus_read(zpg);
    addr.byte.h = bus_read(zpg+1);
    /* TODO(shaw) must handle if the byte at zpg+1 is not in page zero */
    return addr.w;
}

uint16_t am_ind_y(cpu_t *cpu) {
    word_t addr;
    uint8_t zpg = bus_read(cpu->pc++);
    uint16_t sum = cpu->y + bus_read(zpg);
    uint8_t carry = (sum >> 8) != 0; 
    addr.byte.l = sum & 0xFF;
    addr.byte.h = bus_read(zpg+1) + carry;
    return addr.w;
}

uint16_t am_rel(cpu_t *cpu) {
    /* NOTE: signed offset */
    int8_t offset = bus_read(cpu->pc++);
    return cpu->pc + offset;
    /* TODO(shaw) if a page transition occurs, then an extra cycle must
     * be added to execution */
}

uint16_t am_zpg(cpu_t *cpu) {
    return bus_read(cpu->pc++);
}

uint16_t am_zpg_x(cpu_t *cpu) {
    uint8_t addr = cpu->x + bus_read(cpu->pc++);
    return addr;
}

uint16_t am_zpg_y(cpu_t *cpu) {
    uint8_t addr = cpu->y + bus_read(cpu->pc++);
    return addr;
}

/****************************************************************************/
/* operations */
/****************************************************************************/
uint8_t op_adc(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    int same_sign = (cpu->a >> 7) == (operand >> 7);
    uint16_t temp = cpu->a + operand + get_flag(cpu, STATUS_C);
    cpu->a = temp & 0xFF;
    uint8_t overflow = same_sign && (cpu->a >> 7) != (operand >> 7);

    set_flag(cpu, STATUS_C, (temp & 0x100) >> 8);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_V, overflow);
    return 0;
}

uint8_t op_and(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_asl(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bcc(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bcs(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_beq(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bit(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bmi(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bne(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bpl(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}

uint8_t op_brk(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    /* TODO(shaw): this is just a hack to get the repl working, need to actually implement BRK */
    cpu->running = 0;
    return 0;
}

uint8_t op_bvc(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_bvs(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}

uint8_t op_clc(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_C, 0);
    return 0;
}

uint8_t op_cld(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cli(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_clv(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cmp(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cpx(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_cpy(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_dec(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_dex(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_dey(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_eor(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_inc(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_inx(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_iny(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}

uint8_t op_jmp(cpu_t *cpu, uint16_t addr) {
    cpu->pc = addr;
    return 0;
}

uint8_t op_jsr(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}

uint8_t op_lda(cpu_t *cpu, uint16_t addr) {
    cpu->a = bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_ldx(cpu_t *cpu, uint16_t addr) {
    cpu->x = bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_ldy(cpu_t *cpu, uint16_t addr) {
    cpu->y = bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->y >> 7);
    set_flag(cpu, STATUS_Z, cpu->y == 0);
    return 0;
}

uint8_t op_lsr(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_nop(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_ora(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_pha(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_php(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_pla(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_plp(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_rol(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_ror(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_rti(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_rts(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}

uint8_t op_sbc(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t same_sign = (cpu->a & 0x80) == ((~operand) & 0x80);
    uint8_t carry = get_flag(cpu, STATUS_C);
    uint16_t temp = cpu->a + ~operand + carry;
    uint8_t set_carry = cpu->a >= operand - !carry;
    cpu->a = temp & 0xFF;
    uint8_t overflow = same_sign && (cpu->a & 0x80) != ((~operand) & 0x80);

    set_flag(cpu, STATUS_C, set_carry);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_V, overflow);
    return 0;
}

uint8_t op_sec(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_C, 1);
    return 0;
}

uint8_t op_sed(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}
uint8_t op_sei(cpu_t *cpu, uint16_t addr){
    (void)cpu; (void)addr;
    assert(0 && "not implemented");
    return 0;
}

uint8_t op_sta(cpu_t *cpu, uint16_t addr) {
    bus_write(addr, cpu->a);
    return 0;
}

uint8_t op_stx(cpu_t *cpu, uint16_t addr) {
    bus_write(addr, cpu->x);
    return 0;
}

uint8_t op_sty(cpu_t *cpu, uint16_t addr) {
    bus_write(addr, cpu->y);
    return 0;
}

uint8_t op_tax(cpu_t *cpu, uint16_t addr) {
    (void) addr; /* implied */
    cpu->x = cpu->a;
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_tay(cpu_t *cpu, uint16_t addr) {
    (void) addr; /* implied */
    cpu->y = cpu->a;
    set_flag(cpu, STATUS_N, cpu->y >> 7);
    set_flag(cpu, STATUS_Z, cpu->y == 0);
    return 0;
}

uint8_t op_tsx(cpu_t *cpu, uint16_t addr) {
    (void) addr; /* implied */
    cpu->x = cpu->sp;
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_txa(cpu_t *cpu, uint16_t addr) {
    (void) addr; /* implied */
    cpu->a = cpu->x;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_txs(cpu_t *cpu, uint16_t addr) {
    (void) addr; /* implied */
    cpu->sp = cpu->x;
    return 0;
}
 
uint8_t op_tya(cpu_t *cpu, uint16_t addr) {
    (void) addr; /* implied */
    cpu->a = cpu->y;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}


uint8_t op_xxx(cpu_t *cpu, uint16_t addr) {
    (void)cpu; 
    (void)addr;
    printf("Illegal opcode used\n");
    assert(0 && "not reached");
}


op_t ops[OP_COUNT] = 
{
    { "BRK", 7, &op_brk, &am_imm },{ "ORA", 6, &op_ora, &am_x_ind },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 3, &op_nop, &am_imp },{ "ORA", 3, &op_ora, &am_zpg },{ "ASL", 5, &op_asl, &am_zpg },{ "xxx", 5, &op_xxx, &am_imp },{ "PHP", 3, &op_php, &am_imp },{ "ORA", 2, &op_ora, &am_imm },{ "ASL", 2, &op_asl, &am_imp },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "ORA", 4, &op_ora, &am_abs },{ "ASL", 6, &op_asl, &am_abs },{ "xxx", 6, &op_xxx, &am_imp },
    { "BPL", 2, &op_bpl, &am_rel },{ "ORA", 5, &op_ora, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "ORA", 4, &op_ora, &am_zpg_x },{ "ASL", 6, &op_asl, &am_zpg_x },{ "xxx", 6, &op_xxx, &am_imp },{ "CLC", 2, &op_clc, &am_imp },{ "ORA", 4, &op_ora, &am_abs_y },{ "xxx", 2, &op_nop, &am_imp },{ "xxx", 7, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "ORA", 4, &op_ora, &am_abs_x },{ "ASL", 7, &op_asl, &am_abs_x },{ "xxx", 7, &op_xxx, &am_imp },
    { "JSR", 6, &op_jsr, &am_abs },{ "AND", 6, &op_and, &am_x_ind },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "BIT", 3, &op_bit, &am_zpg },{ "AND", 3, &op_and, &am_zpg },{ "ROL", 5, &op_rol, &am_zpg },{ "xxx", 5, &op_xxx, &am_imp },{ "PLP", 4, &op_plp, &am_imp },{ "AND", 2, &op_and, &am_imm },{ "ROL", 2, &op_rol, &am_imp },{ "xxx", 2, &op_xxx, &am_imp },{ "BIT", 4, &op_bit, &am_abs },{ "AND", 4, &op_and, &am_abs },{ "ROL", 6, &op_rol, &am_abs },{ "xxx", 6, &op_xxx, &am_imp },
    { "BMI", 2, &op_bmi, &am_rel },{ "AND", 5, &op_and, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "AND", 4, &op_and, &am_zpg_x },{ "ROL", 6, &op_rol, &am_zpg_x },{ "xxx", 6, &op_xxx, &am_imp },{ "SEC", 2, &op_sec, &am_imp },{ "AND", 4, &op_and, &am_abs_y },{ "xxx", 2, &op_nop, &am_imp },{ "xxx", 7, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "AND", 4, &op_and, &am_abs_x },{ "ROL", 7, &op_rol, &am_abs_x },{ "xxx", 7, &op_xxx, &am_imp },
    { "RTI", 6, &op_rti, &am_imp },{ "EOR", 6, &op_eor, &am_x_ind },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 3, &op_nop, &am_imp },{ "EOR", 3, &op_eor, &am_zpg },{ "LSR", 5, &op_lsr, &am_zpg },{ "xxx", 5, &op_xxx, &am_imp },{ "PHA", 3, &op_pha, &am_imp },{ "EOR", 2, &op_eor, &am_imm },{ "LSR", 2, &op_lsr, &am_imp },{ "xxx", 2, &op_xxx, &am_imp },{ "JMP", 3, &op_jmp, &am_abs },{ "EOR", 4, &op_eor, &am_abs },{ "LSR", 6, &op_lsr, &am_abs },{ "xxx", 6, &op_xxx, &am_imp },
    { "BVC", 2, &op_bvc, &am_rel },{ "EOR", 5, &op_eor, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "EOR", 4, &op_eor, &am_zpg_x },{ "LSR", 6, &op_lsr, &am_zpg_x },{ "xxx", 6, &op_xxx, &am_imp },{ "CLI", 2, &op_cli, &am_imp },{ "EOR", 4, &op_eor, &am_abs_y },{ "xxx", 2, &op_nop, &am_imp },{ "xxx", 7, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "EOR", 4, &op_eor, &am_abs_x },{ "LSR", 7, &op_lsr, &am_abs_x },{ "xxx", 7, &op_xxx, &am_imp },
    { "RTS", 6, &op_rts, &am_imp },{ "ADC", 6, &op_adc, &am_x_ind },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 3, &op_nop, &am_imp },{ "ADC", 3, &op_adc, &am_zpg },{ "ROR", 5, &op_ror, &am_zpg },{ "xxx", 5, &op_xxx, &am_imp },{ "PLA", 4, &op_pla, &am_imp },{ "ADC", 2, &op_adc, &am_imm },{ "ROR", 2, &op_ror, &am_imp },{ "xxx", 2, &op_xxx, &am_imp },{ "JMP", 5, &op_jmp, &am_ind },{ "ADC", 4, &op_adc, &am_abs },{ "ROR", 6, &op_ror, &am_abs },{ "xxx", 6, &op_xxx, &am_imp },
    { "BVS", 2, &op_bvs, &am_rel },{ "ADC", 5, &op_adc, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "ADC", 4, &op_adc, &am_zpg_x },{ "ROR", 6, &op_ror, &am_zpg_x },{ "xxx", 6, &op_xxx, &am_imp },{ "SEI", 2, &op_sei, &am_imp },{ "ADC", 4, &op_adc, &am_abs_y },{ "xxx", 2, &op_nop, &am_imp },{ "xxx", 7, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "ADC", 4, &op_adc, &am_abs_x },{ "ROR", 7, &op_ror, &am_abs_x },{ "xxx", 7, &op_xxx, &am_imp },
    { "xxx", 2, &op_nop, &am_imp },{ "STA", 6, &op_sta, &am_x_ind },{ "xxx", 2, &op_nop, &am_imp },{ "xxx", 6, &op_xxx, &am_imp },{ "STY", 3, &op_sty, &am_zpg },{ "STA", 3, &op_sta, &am_zpg },{ "STX", 3, &op_stx, &am_zpg },{ "xxx", 3, &op_xxx, &am_imp },{ "DEY", 2, &op_dey, &am_imp },{ "xxx", 2, &op_nop, &am_imp },{ "TXA", 2, &op_txa, &am_imp },{ "xxx", 2, &op_xxx, &am_imp },{ "STY", 4, &op_sty, &am_abs },{ "STA", 4, &op_sta, &am_abs },{ "STX", 4, &op_stx, &am_abs },{ "xxx", 4, &op_xxx, &am_imp },
    { "BCC", 2, &op_bcc, &am_rel },{ "STA", 6, &op_sta, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 6, &op_xxx, &am_imp },{ "STY", 4, &op_sty, &am_zpg_x },{ "STA", 4, &op_sta, &am_zpg_x },{ "STX", 4, &op_stx, &am_zpg_y },{ "xxx", 4, &op_xxx, &am_imp },{ "TYA", 2, &op_tya, &am_imp },{ "STA", 5, &op_sta, &am_abs_y },{ "TXS", 2, &op_txs, &am_imp },{ "xxx", 5, &op_xxx, &am_imp },{ "xxx", 5, &op_nop, &am_imp },{ "STA", 5, &op_sta, &am_abs_x },{ "xxx", 5, &op_xxx, &am_imp },{ "xxx", 5, &op_xxx, &am_imp },
    { "LDY", 2, &op_ldy, &am_imm },{ "LDA", 6, &op_lda, &am_x_ind },{ "LDX", 2, &op_ldx, &am_imm },{ "xxx", 6, &op_xxx, &am_imp },{ "LDY", 3, &op_ldy, &am_zpg },{ "LDA", 3, &op_lda, &am_zpg },{ "LDX", 3, &op_ldx, &am_zpg },{ "xxx", 3, &op_xxx, &am_imp },{ "TAY", 2, &op_tay, &am_imp },{ "LDA", 2, &op_lda, &am_imm },{ "TAX", 2, &op_tax, &am_imp },{ "xxx", 2, &op_xxx, &am_imp },{ "LDY", 4, &op_ldy, &am_abs },{ "LDA", 4, &op_lda, &am_abs },{ "LDX", 4, &op_ldx, &am_abs },{ "xxx", 4, &op_xxx, &am_imp },
    { "BCS", 2, &op_bcs, &am_rel },{ "LDA", 5, &op_lda, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 5, &op_xxx, &am_imp },{ "LDY", 4, &op_ldy, &am_zpg_x },{ "LDA", 4, &op_lda, &am_zpg_x },{ "LDX", 4, &op_ldx, &am_zpg_y },{ "xxx", 4, &op_xxx, &am_imp },{ "CLV", 2, &op_clv, &am_imp },{ "LDA", 4, &op_lda, &am_abs_y },{ "TSX", 2, &op_tsx, &am_imp },{ "xxx", 4, &op_xxx, &am_imp },{ "LDY", 4, &op_ldy, &am_abs_x },{ "LDA", 4, &op_lda, &am_abs_x },{ "LDX", 4, &op_ldx, &am_abs_y },{ "xxx", 4, &op_xxx, &am_imp },
    { "CPY", 2, &op_cpy, &am_imm },{ "CMP", 6, &op_cmp, &am_x_ind },{ "xxx", 2, &op_nop, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "CPY", 3, &op_cpy, &am_zpg },{ "CMP", 3, &op_cmp, &am_zpg },{ "DEC", 5, &op_dec, &am_zpg },{ "xxx", 5, &op_xxx, &am_imp },{ "INY", 2, &op_iny, &am_imp },{ "CMP", 2, &op_cmp, &am_imm },{ "DEX", 2, &op_dex, &am_imp },{ "xxx", 2, &op_xxx, &am_imp },{ "CPY", 4, &op_cpy, &am_abs },{ "CMP", 4, &op_cmp, &am_abs },{ "DEC", 6, &op_dec, &am_abs },{ "xxx", 6, &op_xxx, &am_imp },
    { "BNE", 2, &op_bne, &am_rel },{ "CMP", 5, &op_cmp, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "CMP", 4, &op_cmp, &am_zpg_x },{ "DEC", 6, &op_dec, &am_zpg_x },{ "xxx", 6, &op_xxx, &am_imp },{ "CLD", 2, &op_cld, &am_imp },{ "CMP", 4, &op_cmp, &am_abs_y },{ "NOP", 2, &op_nop, &am_imp },{ "xxx", 7, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "CMP", 4, &op_cmp, &am_abs_x },{ "DEC", 7, &op_dec, &am_abs_x },{ "xxx", 7, &op_xxx, &am_imp },
    { "CPX", 2, &op_cpx, &am_imm },{ "SBC", 6, &op_sbc, &am_x_ind },{ "xxx", 2, &op_nop, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "CPX", 3, &op_cpx, &am_zpg },{ "SBC", 3, &op_sbc, &am_zpg },{ "INC", 5, &op_inc, &am_zpg },{ "xxx", 5, &op_xxx, &am_imp },{ "INX", 2, &op_inx, &am_imp },{ "SBC", 2, &op_sbc, &am_imm },{ "NOP", 2, &op_nop, &am_imp },{ "xxx", 2, &op_sbc, &am_imp },{ "CPX", 4, &op_cpx, &am_abs },{ "SBC", 4, &op_sbc, &am_abs },{ "INC", 6, &op_inc, &am_abs },{ "xxx", 6, &op_xxx, &am_imp },
    { "BEQ", 2, &op_beq, &am_rel },{ "SBC", 5, &op_sbc, &am_ind_y },{ "xxx", 2, &op_xxx, &am_imp },{ "xxx", 8, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "SBC", 4, &op_sbc, &am_zpg_x },{ "INC", 6, &op_inc, &am_zpg_x },{ "xxx", 6, &op_xxx, &am_imp },{ "SED", 2, &op_sed, &am_imp },{ "SBC", 4, &op_sbc, &am_abs_y },{ "NOP", 2, &op_nop, &am_imp },{ "xxx", 7, &op_xxx, &am_imp },{ "xxx", 4, &op_nop, &am_imp },{ "SBC", 4, &op_sbc, &am_abs_x },{ "INC", 7, &op_inc, &am_abs_x },{ "xxx", 7, &op_xxx, &am_imp }
};


void cpu_reset(cpu_t *cpu) {
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFF;
    cpu->status = 1 << 5;

    /* TODO(shaw): the reset vector is 0xFFFC, 0xFFFD on startup the cpu would
     * read the values at these locations into pc and perform a JMP. For now
     * i'm just hardcoding the pc to a value */

    /*cpu->pc = 0xFFFC;*/
    cpu->pc = 0xC000;

    cpu->interrupt_period = 1;
    cpu->cycle_counter = cpu->interrupt_period;

    cpu->running = 1;
}

void cpu_tick(cpu_t *cpu) {
    uint8_t opcode = bus_read(cpu->pc++);
    op_t op = ops[opcode];
    uint16_t addr = op.addr_mode(cpu);
    op.execute(cpu, addr);
}

int cpu_run(cpu_t *cpu) {
    uint8_t opcode;
    uint16_t addr;
    op_t op;
    cpu->running = 1;

    while (cpu->running) {
        opcode = bus_read(cpu->pc++);
        op = ops[opcode];
        addr = op.addr_mode(cpu);
        op.execute(cpu, addr);

        /*cpu->cycle_counter -= op.cycles;*/

        /*if (cpu->cycle_counter <= 0) {*/
            /*[> Check for interrupts and do other <]*/
            /*[> cyclic tasks here                 <]*/

            /*cpu->cycle_counter += cpu->interrupt_period;*/
            /*if (exit_required) break;*/
        /*}*/
    }

    return 0;
}

