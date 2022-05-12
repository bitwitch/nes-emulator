#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu_6502.h"
#include "stb_ds.h"

#ifdef DEBUG_LOG
FILE *logfile;
#endif

typedef union {
    uint16_t w;
#ifdef CPU6502_BIG_ENDIAN
    struct { uint8_t h,l; } byte;
#else
    struct { uint8_t l,h; } byte;
#endif
} word_t;


/****************************************************************************/
/* interrupts */
/****************************************************************************/
void cpu_irq(cpu_t *cpu) {
    if (get_flag(cpu, STATUS_I))
        return;

    /* push pc */
    word_t return_addr = { .w = cpu->pc };
    bus_write(0x100 | cpu->sp, return_addr.byte.h);
    --cpu->sp;
    bus_write(0x100 | cpu->sp, return_addr.byte.l);
    --cpu->sp;

    /* push status */
    bus_write(0x100 | cpu->sp, (cpu->status|STATUS_U) & ~STATUS_B);
    --cpu->sp;

    /* jump to interrupt handler */
    cpu->pc = bus_read(0xFFFE) | (bus_read(0xFFFF) << 8);

    /* set interrupt disable */
    set_flag(cpu, STATUS_I, 1);
}

void cpu_nmi(cpu_t *cpu) {
    /* push pc */
    word_t return_addr = { .w = cpu->pc };
    bus_write(0x100 | cpu->sp, return_addr.byte.h);
    --cpu->sp;
    bus_write(0x100 | cpu->sp, return_addr.byte.l);
    --cpu->sp;

    /* push status */
    bus_write(0x100 | cpu->sp, (cpu->status|STATUS_U) & ~STATUS_B);
    --cpu->sp;

    /* jump to interrupt handler */
    cpu->pc = bus_read(0xFFFA) | (bus_read(0xFFFB) << 8);

    /* set interrupt disable */
    set_flag(cpu, STATUS_I, 1);

    cpu->op_cycles = 7;
}

/****************************************************************************/
/* Address Modes
 *
 * Each address mode takes an address as an out parameter, and returns 1 if
 * there is potentially an additional cycle needed because of a page boundary
 * crossing, or zero otherwise. This result will be combined with the
 * additional cycle flag returned by the operation function to determine if an
 * additional cycle is actually added or not
*/
/****************************************************************************/
uint8_t am_imp(cpu_t *cpu, uint16_t *addr) {
    (void)cpu; 
    (void)addr;
    return 0; 
}

uint8_t am_abs(cpu_t *cpu, uint16_t *addr) {
    word_t result;
    result.byte.l = bus_read(cpu->pc++);
    result.byte.h = bus_read(cpu->pc++);
    *addr = result.w;
    return 0;
}

uint8_t am_abx(cpu_t *cpu, uint16_t *addr) {
    word_t base_addr;
    base_addr.byte.l = bus_read(cpu->pc++);
    base_addr.byte.h = bus_read(cpu->pc++);
    *addr = base_addr.w + cpu->x;

    if ((base_addr.w & 0xFF00) != (*addr & 0xFF00))
        return 1;

    return 0;
}

uint8_t am_aby(cpu_t *cpu, uint16_t *addr) {
    word_t base_addr;
    base_addr.byte.l = bus_read(cpu->pc++);
    base_addr.byte.h = bus_read(cpu->pc++);
    *addr = base_addr.w + cpu->y;

    if ((base_addr.w & 0xFF00) != (*addr & 0xFF00))
        return 1;

    return 0;
}

uint8_t am_imm(cpu_t *cpu, uint16_t *addr) {
    *addr = cpu->pc++;
    return 0;
}

/*
 * The 6502 had a hardware bug with the indirect addressing, if on a page boundary
 * like $xxFF would wrap around to $xx00 instead of the intended crossing of
 * the page boundary.  see http://archive.6502.org/publications/6502notes/6502_user_notes_15.pdf#page=24
 */
uint8_t am_ind(cpu_t *cpu, uint16_t *addr) {
    word_t result, pointer;
    pointer.byte.l = bus_read(cpu->pc++);
    pointer.byte.h = bus_read(cpu->pc++);
    result.byte.l = bus_read(pointer.w);
    uint16_t ptr_high_byte = pointer.byte.l == 0xFF ? pointer.w & 0xFF00 : pointer.w + 1;
    result.byte.h = bus_read(ptr_high_byte);
    *addr = result.w;
    return 0;
}

uint8_t am_x_ind(cpu_t *cpu, uint16_t *addr) {
    word_t result;
    uint8_t zpg = cpu->x + bus_read(cpu->pc++);
    result.byte.l = bus_read(zpg++);
    result.byte.h = bus_read(zpg);
    *addr = result.w;
    return 0;
}

uint8_t am_ind_y(cpu_t *cpu, uint16_t *addr) {
    word_t pointer;
    uint8_t zpg = bus_read(cpu->pc++);
    pointer.byte.l = bus_read(zpg);
    ++zpg;
    pointer.byte.h = bus_read(zpg);
    *addr = pointer.w + cpu->y;

    if ((pointer.w & 0xFF00) != (*addr & 0xFF00))
        return 1;

    return 0;
}

uint8_t am_rel(cpu_t *cpu, uint16_t *addr) {
    /* NOTE: signed offset */
    int8_t offset = bus_read(cpu->pc++);
    *addr = cpu->pc + offset;
    if ((cpu->pc & 0xFF00) != (*addr & 0xFF00))
        return 1;
    return 0;
}

uint8_t am_zpg(cpu_t *cpu, uint16_t *addr) {
    *addr = bus_read(cpu->pc++);
    return 0;
}

uint8_t am_zpx(cpu_t *cpu, uint16_t *addr) {
    *addr = cpu->x + bus_read(cpu->pc++);
    *addr &= 0xFF; 
    return 0;
}

uint8_t am_zpy(cpu_t *cpu, uint16_t *addr) {
    *addr = cpu->y + bus_read(cpu->pc++);
    *addr &= 0xFF; 
    return 0;
}

/****************************************************************************/
/* Operations 
 *
 * Return 1 if there is potentially an additional cycle needed or zero
 * otherwise. This result will be combined with the additional cycle flag
 * returned by the address mode to determine if an additional cycle is actually
 * needed or not
*/
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
    return 1;
}

uint8_t op_and(cpu_t *cpu, uint16_t addr) {
    cpu->a &= bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 1;
}

uint8_t op_asl(cpu_t *cpu, uint16_t addr) {
    if (cpu->opcode == 0x0A) {
        /* accumulator mode */
        set_flag(cpu, STATUS_C, cpu->a >> 7);
        cpu->a <<= 1;
        set_flag(cpu, STATUS_N, cpu->a >> 7);
        set_flag(cpu, STATUS_Z, cpu->a == 0);
    } else { 
        /* memory mode */
        uint8_t operand = bus_read(addr);
        uint8_t result = operand << 1;
        bus_write(addr, result);
        set_flag(cpu, STATUS_C, operand >> 7);
        set_flag(cpu, STATUS_N, result >> 7);
        set_flag(cpu, STATUS_Z, result == 0);
    }

    return 0;
}

uint8_t op_bcc(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_C))
        return 0;
    ++cpu->op_cycles;
    cpu->pc = addr;
    return 1;
}

uint8_t op_bcs(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_C)) {
        ++cpu->op_cycles;
        cpu->pc = addr;
        return 1;
    }
    return 0;
}

uint8_t op_beq(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_Z)) {
        ++cpu->op_cycles;
        cpu->pc = addr;
        return 1;
    }
    return 0;
}

uint8_t op_bit(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    /* set bits 7 and 6 of status reg (N,V) to bits 7 and 6 of operand */
    cpu->status = (cpu->status & ~0xC0) | (operand & 0xC0);
    set_flag(cpu, STATUS_Z, !(cpu->a & operand));
    return 0;
}

uint8_t op_bmi(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_N)) {
        ++cpu->op_cycles;
        cpu->pc = addr;
        return 1;
    }
    return 0;
}

uint8_t op_bne(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_Z))
        return 0;
    ++cpu->op_cycles;
    cpu->pc = addr;
    return 1;
}

uint8_t op_bpl(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_N))
        return 0;
    ++cpu->op_cycles;
    cpu->pc = addr;
    return 1;
}

uint8_t op_brk(cpu_t *cpu, uint16_t addr) {
    (void)addr;

    /* TODO(shaw): this is just a hack to get the repl working, need to actually implement BRK */
    /*cpu->running = false;*/

    /* push pc+2 (two bytes after the opcode, leaving one byte for a break mark 
     * one is used below because the opcode has already been read */
    word_t return_addr = { .w = cpu->pc+1 };
    bus_write(0x100 | cpu->sp, return_addr.byte.h);
    --cpu->sp;
    bus_write(0x100 | cpu->sp, return_addr.byte.l);
    --cpu->sp;

    /* push status, with break flag set */
    bus_write(0x100 | cpu->sp, cpu->status|STATUS_U|STATUS_B);
    --cpu->sp;

    /* jump to interrupt handler */
    cpu->pc = bus_read(0xFFFE) | (bus_read(0xFFFF) << 8);

    /* set interrupt disable */
    set_flag(cpu, STATUS_I, 1);

    return 0;
}

uint8_t op_bvc(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_V)) 
        return 0;
    ++cpu->op_cycles;
    cpu->pc = addr;
    return 1;
}

uint8_t op_bvs(cpu_t *cpu, uint16_t addr) {
    if (get_flag(cpu, STATUS_V)) {
        ++cpu->op_cycles;
        cpu->pc = addr;
        return 1;
    }
    return 0;
}

uint8_t op_clc(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_C, 0);
    return 0;
}

uint8_t op_cld(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_D, 0);
    return 0;
}

uint8_t op_cli(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_I, 0);
    return 0;
}

uint8_t op_clv(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_V, 0);
    return 0;
}

uint8_t op_cmp(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t result = cpu->a - operand;
    set_flag(cpu, STATUS_C, cpu->a >= operand);
    set_flag(cpu, STATUS_N, result >> 7);
    set_flag(cpu, STATUS_Z, result == 0);
    return 1;
}

uint8_t op_cpx(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t result = cpu->x - operand;
    set_flag(cpu, STATUS_C, cpu->x >= operand);
    set_flag(cpu, STATUS_N, result >> 7);
    set_flag(cpu, STATUS_Z, result == 0);
    return 0;
}

uint8_t op_cpy(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t result = cpu->y - operand;
    set_flag(cpu, STATUS_C, cpu->y >= operand);
    set_flag(cpu, STATUS_N, result >> 7);
    set_flag(cpu, STATUS_Z, result == 0);
    return 0;
}

uint8_t op_dec(cpu_t *cpu, uint16_t addr) {
    uint8_t result = bus_read(addr) - 1;
    bus_write(addr, result);
    set_flag(cpu, STATUS_N, result >> 7);
    set_flag(cpu, STATUS_Z, result == 0);
    return 0;
}

uint8_t op_dex(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    --cpu->x;
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_dey(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    --cpu->y;
    set_flag(cpu, STATUS_N, cpu->y >> 7);
    set_flag(cpu, STATUS_Z, cpu->y == 0);
    return 0;
}

uint8_t op_eor(cpu_t *cpu, uint16_t addr) {
    cpu->a ^= bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 1;
}

uint8_t op_inc(cpu_t *cpu, uint16_t addr) {
    uint8_t result = bus_read(addr) + 1;
    bus_write(addr, result);
    set_flag(cpu, STATUS_N, result >> 7);
    set_flag(cpu, STATUS_Z, result == 0);
    return 0;
}

uint8_t op_inx(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    ++cpu->x;
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_iny(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    ++cpu->y;
    set_flag(cpu, STATUS_N, cpu->y >> 7);
    set_flag(cpu, STATUS_Z, cpu->y == 0);
    return 0;
}

uint8_t op_jmp(cpu_t *cpu, uint16_t addr) {
    cpu->pc = addr;
    return 0;
}

/* 
 * we want to push pc+2 on the stack (the third byte of the jsr instruction)
 * pc was already incremented once for the opcode read
 * pc was then incremented twice for the address read by am_abs()
 * so to get the third byte we actually have to push pc - 1 on the stack
 */
uint8_t op_jsr(cpu_t *cpu, uint16_t addr) {
    word_t return_addr = { .w = cpu->pc - 1 };
    bus_write(0x100 | cpu->sp, return_addr.byte.h);
    --cpu->sp;
    bus_write(0x100 | cpu->sp, return_addr.byte.l);
    --cpu->sp;
    cpu->pc = addr;
    return 0;
}

uint8_t op_lda(cpu_t *cpu, uint16_t addr) {
    cpu->a = bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 1;
}

uint8_t op_ldx(cpu_t *cpu, uint16_t addr) {
    cpu->x = bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 1;
}

uint8_t op_ldy(cpu_t *cpu, uint16_t addr) {
    cpu->y = bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->y >> 7);
    set_flag(cpu, STATUS_Z, cpu->y == 0);
    return 1;
}

uint8_t op_lsr(cpu_t *cpu, uint16_t addr) {
    if (cpu->opcode == 0x4A) {
        /* accumulator mode */
        set_flag(cpu, STATUS_C, cpu->a & 1);
        cpu->a >>= 1;
        set_flag(cpu, STATUS_N, cpu->a >> 7);
        set_flag(cpu, STATUS_Z, cpu->a == 0);
    } else { 
        /* memory mode */
        uint8_t operand = bus_read(addr);
        uint8_t result = operand >> 1;
        bus_write(addr, result);
        set_flag(cpu, STATUS_C, operand & 1);
        set_flag(cpu, STATUS_N, result >> 7);
        set_flag(cpu, STATUS_Z, result == 0);
    }
    return 0;
}

uint8_t op_nop(cpu_t *cpu, uint16_t addr) {
    (void)cpu; (void)addr;
    return 1;
}

uint8_t op_ora(cpu_t *cpu, uint16_t addr) {
    cpu->a |= bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 1;
}

uint8_t op_pha(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    bus_write(0x100 | cpu->sp, cpu->a);
    --cpu->sp;
    return 0;
}

uint8_t op_php(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    bus_write(0x100 | cpu->sp, cpu->status | STATUS_U | STATUS_B );
    --cpu->sp;
    return 0;
}

uint8_t op_pla(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    cpu->a = bus_read(0x100 | ++cpu->sp);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_plp(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    /*cpu->status = bus_read(0x100 | ++cpu->sp) | STATUS_U | STATUS_B;*/
    cpu->status = (bus_read(0x100 | ++cpu->sp) | STATUS_U) & ~STATUS_B;
    return 0;
}

uint8_t op_rol(cpu_t *cpu, uint16_t addr) {
    uint8_t old_c = get_flag(cpu, STATUS_C);
    if (cpu->opcode == 0x2A) {
        /* accumulator mode */
        set_flag(cpu, STATUS_C, cpu->a >> 7);
        cpu->a = (cpu->a << 1) | old_c;
        set_flag(cpu, STATUS_N, cpu->a >> 7);
        set_flag(cpu, STATUS_Z, cpu->a == 0);
    } else { 
        /* memory mode */
        uint8_t operand = bus_read(addr);
        uint8_t result = (operand << 1) | old_c;
        bus_write(addr, result);
        set_flag(cpu, STATUS_C, operand >> 7);
        set_flag(cpu, STATUS_N, result >> 7);
        set_flag(cpu, STATUS_Z, result == 0);
    }
    return 0;
}

uint8_t op_ror(cpu_t *cpu, uint16_t addr) {
    uint8_t old_c = get_flag(cpu, STATUS_C);
    if (cpu->opcode == 0x6A) {
        /* accumulator mode */
        set_flag(cpu, STATUS_C, cpu->a & 1);
        cpu->a = (old_c << 7) | (cpu->a >> 1);
        set_flag(cpu, STATUS_N, cpu->a >> 7);
        set_flag(cpu, STATUS_Z, cpu->a == 0);
    } else { 
        /* memory mode */
        uint8_t operand = bus_read(addr);
        uint8_t result = (old_c << 7) | (operand >> 1);
        bus_write(addr, result);
        set_flag(cpu, STATUS_C, operand & 1);
        set_flag(cpu, STATUS_N, result >> 7);
        set_flag(cpu, STATUS_Z, result == 0);
    }
    return 0;
}


/* NOTE(shaw): see https://www.nesdev.org/wiki/Status_flags#The_B_flag
 * for more info regarding rti ignoring bits 4 and 5 of the
 * status pulled from the stack
 */
uint8_t op_rti(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    word_t temp;
    /*cpu->status = bus_read(0x100 | ++cpu->sp) | STATUS_U | STATUS_B;*/
    cpu->status = (bus_read(0x100 | ++cpu->sp) | STATUS_U) & ~STATUS_B;
    temp.byte.l = bus_read(0x100 | ++cpu->sp);
    temp.byte.h = bus_read(0x100 | ++cpu->sp);
    cpu->pc = temp.w;
    return 0;
}

uint8_t op_rts(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    word_t temp;
    temp.byte.l = bus_read(0x100 | ++cpu->sp);
    temp.byte.h = bus_read(0x100 | ++cpu->sp);
    cpu->pc = temp.w + 1;
    return 0;
}

uint8_t op_sbc(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr) ^ 0xFF;
    uint8_t same_sign = (cpu->a & 0x80) == (operand & 0x80);
    uint16_t temp = cpu->a + operand + get_flag(cpu, STATUS_C);
    cpu->a = temp & 0xFF;
    uint8_t overflow = same_sign && (cpu->a & 0x80) != (operand & 0x80);

    set_flag(cpu, STATUS_C, (temp & 0x100) >> 8);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_V, overflow);
    return 1;
}

uint8_t op_sec(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_C, 1);
    return 0;
}

uint8_t op_sed(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_D, 1);
    return 0;
}

uint8_t op_sei(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    set_flag(cpu, STATUS_I, 1);
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
    (void)addr;
    cpu->x = cpu->a;
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_tay(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    cpu->y = cpu->a;
    set_flag(cpu, STATUS_N, cpu->y >> 7);
    set_flag(cpu, STATUS_Z, cpu->y == 0);
    return 0;
}

uint8_t op_tsx(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    cpu->x = cpu->sp;
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_txa(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    cpu->a = cpu->x;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_txs(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    cpu->sp = cpu->x;

    return 0;
}

uint8_t op_tya(cpu_t *cpu, uint16_t addr) {
    (void)addr;
    cpu->a = cpu->y;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}


/* 
 * Illegal Opcodes 
 */ 
uint8_t op_kil(cpu_t *cpu, uint16_t addr) {
    (void)cpu; 
    (void)addr;
    assert(0 && "kil instruction halted cpu");
    return 0;
}

uint8_t op_slo(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t asl_result = operand << 1;
    bus_write(addr, asl_result);
    set_flag(cpu, STATUS_C, operand >> 7);

    cpu->a |= asl_result;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_rla(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t c = get_flag(cpu, STATUS_C);
    uint8_t rol_result = (operand << 1) | c;
    bus_write(addr, rol_result);
    set_flag(cpu, STATUS_C, operand >> 7);

    cpu->a &= rol_result;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);

    return 0;
}

uint8_t op_sre(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t lsr_result = operand >> 1;
    bus_write(addr, lsr_result);
    set_flag(cpu, STATUS_C, operand & 1);

    cpu->a ^= lsr_result;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_rra(cpu_t *cpu, uint16_t addr) {
    uint8_t operand = bus_read(addr);
    uint8_t c = get_flag(cpu, STATUS_C);
    uint8_t ror_result = (c << 7) | (operand >> 1);
    bus_write(addr, ror_result);
    set_flag(cpu, STATUS_C, operand & 1);

    int same_sign = (cpu->a >> 7) == (ror_result >> 7);
    uint16_t temp = cpu->a + ror_result + get_flag(cpu, STATUS_C);
    cpu->a = temp & 0xFF;
    uint8_t overflow = same_sign && (cpu->a >> 7) != (ror_result >> 7);

    set_flag(cpu, STATUS_C, (temp & 0x100) >> 8);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_V, overflow);
    return 0;
}

uint8_t op_sax(cpu_t *cpu, uint16_t addr) {
    bus_write(addr, cpu->a & cpu->x);
    return 0;
}

uint8_t op_lax(cpu_t *cpu, uint16_t addr) {
    cpu->a = bus_read(addr);
    cpu->x = cpu->a;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 1;
}

uint8_t op_dcp(cpu_t *cpu, uint16_t addr) {
    uint8_t dec_result = bus_read(addr) - 1;
    bus_write(addr, dec_result);
    uint8_t cmp_result = cpu->a - dec_result;
    set_flag(cpu, STATUS_C, cpu->a >= dec_result);
    set_flag(cpu, STATUS_N, cmp_result >> 7);
    set_flag(cpu, STATUS_Z, cmp_result == 0);
    return 0;
}

uint8_t op_isc(cpu_t *cpu, uint16_t addr) {
    uint8_t inc_result = bus_read(addr) + 1;
    bus_write(addr, inc_result);

    inc_result ^= 0xFF;
    uint8_t same_sign = (cpu->a & 0x80) == (inc_result & 0x80);
    uint16_t temp = cpu->a + inc_result + get_flag(cpu, STATUS_C);
    cpu->a = temp & 0xFF;
    uint8_t overflow = same_sign && (cpu->a & 0x80) != (inc_result & 0x80);

    set_flag(cpu, STATUS_C, (temp & 0x100) >> 8);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_V, overflow);
    return 0;
}

uint8_t op_anc(cpu_t *cpu, uint16_t addr) {
    cpu->a &= bus_read(addr);
    set_flag(cpu, STATUS_C, cpu->a >> 7);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_alr(cpu_t *cpu, uint16_t addr) {
    cpu->a &= bus_read(addr);
    set_flag(cpu, STATUS_C, cpu->a & 1);
    cpu->a >>= 1;
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_arr(cpu_t *cpu, uint16_t addr) {
    cpu->a &= bus_read(addr);

    uint8_t c = get_flag(cpu, STATUS_C);
    cpu->a = (c << 7) | (cpu->a >> 1);

    bool bit6 = (cpu->a >> 6) & 1;
    bool bit5 = (cpu->a >> 5) & 1;
    set_flag(cpu, STATUS_C, bit6);
    set_flag(cpu, STATUS_V, bit5^bit6);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_xaa(cpu_t *cpu, uint16_t addr) {
    cpu->a = cpu->x;
    cpu->a &= bus_read(addr);
    set_flag(cpu, STATUS_N, cpu->a >> 7);
    set_flag(cpu, STATUS_Z, cpu->a == 0);
    return 0;
}

uint8_t op_axs(cpu_t *cpu, uint16_t addr) {
    uint8_t imm = bus_read(addr);
    cpu->x &= cpu->a;
    set_flag(cpu, STATUS_C, cpu->x >= imm);

    cpu->x -= imm;
    set_flag(cpu, STATUS_N, cpu->x >> 7);
    set_flag(cpu, STATUS_Z, cpu->x == 0);
    return 0;
}

uint8_t op_ahx(cpu_t *cpu, uint16_t addr) {
    word_t h = { .w = addr+1 };
    bus_write(addr, cpu->a & cpu->x & h.byte.h);
    return 0;
}

uint8_t op_shy(cpu_t *cpu, uint16_t addr) {
    word_t h = { .w = addr };
    bus_write(addr, cpu->y & (h.byte.h + 1));
    return 0;
}

uint8_t op_shx(cpu_t *cpu, uint16_t addr) {
    word_t h = { .w = addr };
    bus_write(addr, cpu->x & (h.byte.h + 1));
    return 0;
}

uint8_t op_tas(cpu_t *cpu, uint16_t addr) {
    word_t h = { .w = addr+1 };
    bus_write(addr, cpu->a & cpu->x & h.byte.h);
    cpu->sp = cpu->x & cpu->a;
    return 0;
}

uint8_t op_las(cpu_t *cpu, uint16_t addr) {
    uint8_t result = cpu->sp & bus_read(addr);
    cpu->a  = result;
    cpu->x  = result;
    cpu->sp = result;
    set_flag(cpu, STATUS_N, result >> 7);
    set_flag(cpu, STATUS_Z, result == 0);
    return 1;
}

op_t ops[OP_COUNT] = 
{
    { "BRK", 7, &op_brk, &am_imm },{ "ORA", 6, &op_ora, &am_x_ind },{ "KIL", 2, &op_kil, &am_imp },{ "SLO", 8, &op_slo, &am_x_ind },{ "NOP", 3, &op_nop, &am_zpg },{ "ORA", 3, &op_ora, &am_zpg },{ "ASL", 5, &op_asl, &am_zpg },{ "SLO", 5, &op_slo, &am_zpg },{ "PHP", 3, &op_php, &am_imp },{ "ORA", 2, &op_ora, &am_imm },{ "ASL", 2, &op_asl, &am_imp },{ "ANC", 2, &op_anc, &am_imm },{ "NOP", 4, &op_nop, &am_abs },{ "ORA", 4, &op_ora, &am_abs },{ "ASL", 6, &op_asl, &am_abs },{ "SLO", 6, &op_slo, &am_abs },
    { "BPL", 2, &op_bpl, &am_rel },{ "ORA", 5, &op_ora, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "SLO", 8, &op_slo, &am_ind_y },{ "NOP", 4, &op_nop, &am_zpx },{ "ORA", 4, &op_ora, &am_zpx },{ "ASL", 6, &op_asl, &am_zpx },{ "SLO", 6, &op_slo, &am_zpx },{ "CLC", 2, &op_clc, &am_imp },{ "ORA", 4, &op_ora, &am_aby },{ "NOP", 2, &op_nop, &am_imp },{ "SLO", 7, &op_slo, &am_aby },{ "NOP", 4, &op_nop, &am_abx },{ "ORA", 4, &op_ora, &am_abx },{ "ASL", 7, &op_asl, &am_abx },{ "SLO", 7, &op_slo, &am_abx },
    { "JSR", 6, &op_jsr, &am_abs },{ "AND", 6, &op_and, &am_x_ind },{ "KIL", 2, &op_kil, &am_imp },{ "RLA", 8, &op_rla, &am_x_ind },{ "BIT", 3, &op_bit, &am_zpg },{ "AND", 3, &op_and, &am_zpg },{ "ROL", 5, &op_rol, &am_zpg },{ "RLA", 5, &op_rla, &am_zpg },{ "PLP", 4, &op_plp, &am_imp },{ "AND", 2, &op_and, &am_imm },{ "ROL", 2, &op_rol, &am_imp },{ "ANC", 2, &op_anc, &am_imm },{ "BIT", 4, &op_bit, &am_abs },{ "AND", 4, &op_and, &am_abs },{ "ROL", 6, &op_rol, &am_abs },{ "RLA", 6, &op_rla, &am_abs },
    { "BMI", 2, &op_bmi, &am_rel },{ "AND", 5, &op_and, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "RLA", 8, &op_rla, &am_ind_y },{ "NOP", 4, &op_nop, &am_zpx },{ "AND", 4, &op_and, &am_zpx },{ "ROL", 6, &op_rol, &am_zpx },{ "RLA", 6, &op_rla, &am_zpx },{ "SEC", 2, &op_sec, &am_imp },{ "AND", 4, &op_and, &am_aby },{ "NOP", 2, &op_nop, &am_imp },{ "RLA", 7, &op_rla, &am_aby },{ "NOP", 4, &op_nop, &am_abx },{ "AND", 4, &op_and, &am_abx },{ "ROL", 7, &op_rol, &am_abx },{ "RLA", 7, &op_rla, &am_abx },
    { "RTI", 6, &op_rti, &am_imp },{ "EOR", 6, &op_eor, &am_x_ind },{ "KIL", 2, &op_kil, &am_imp },{ "SRE", 8, &op_sre, &am_x_ind },{ "NOP", 3, &op_nop, &am_zpg },{ "EOR", 3, &op_eor, &am_zpg },{ "LSR", 5, &op_lsr, &am_zpg },{ "SRE", 5, &op_sre, &am_zpg },{ "PHA", 3, &op_pha, &am_imp },{ "EOR", 2, &op_eor, &am_imm },{ "LSR", 2, &op_lsr, &am_imp },{ "ALR", 2, &op_alr, &am_imm },{ "JMP", 3, &op_jmp, &am_abs },{ "EOR", 4, &op_eor, &am_abs },{ "LSR", 6, &op_lsr, &am_abs },{ "SRE", 6, &op_sre, &am_abs },
    { "BVC", 2, &op_bvc, &am_rel },{ "EOR", 5, &op_eor, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "SRE", 8, &op_sre, &am_ind_y },{ "NOP", 4, &op_nop, &am_zpx },{ "EOR", 4, &op_eor, &am_zpx },{ "LSR", 6, &op_lsr, &am_zpx },{ "SRE", 6, &op_sre, &am_zpx },{ "CLI", 2, &op_cli, &am_imp },{ "EOR", 4, &op_eor, &am_aby },{ "NOP", 2, &op_nop, &am_imp },{ "SRE", 7, &op_sre, &am_aby },{ "NOP", 4, &op_nop, &am_abx },{ "EOR", 4, &op_eor, &am_abx },{ "LSR", 7, &op_lsr, &am_abx },{ "SRE", 7, &op_sre, &am_abx },
    { "RTS", 6, &op_rts, &am_imp },{ "ADC", 6, &op_adc, &am_x_ind },{ "KIL", 2, &op_kil, &am_imp },{ "RRA", 8, &op_rra, &am_x_ind },{ "NOP", 3, &op_nop, &am_zpg },{ "ADC", 3, &op_adc, &am_zpg },{ "ROR", 5, &op_ror, &am_zpg },{ "RRA", 5, &op_rra, &am_zpg },{ "PLA", 4, &op_pla, &am_imp },{ "ADC", 2, &op_adc, &am_imm },{ "ROR", 2, &op_ror, &am_imp },{ "ARR", 2, &op_arr, &am_imm },{ "JMP", 5, &op_jmp, &am_ind },{ "ADC", 4, &op_adc, &am_abs },{ "ROR", 6, &op_ror, &am_abs },{ "RRA", 6, &op_rra, &am_abs },
    { "BVS", 2, &op_bvs, &am_rel },{ "ADC", 5, &op_adc, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "RRA", 8, &op_rra, &am_ind_y },{ "NOP", 4, &op_nop, &am_zpx },{ "ADC", 4, &op_adc, &am_zpx },{ "ROR", 6, &op_ror, &am_zpx },{ "RRA", 6, &op_rra, &am_zpx },{ "SEI", 2, &op_sei, &am_imp },{ "ADC", 4, &op_adc, &am_aby },{ "NOP", 2, &op_nop, &am_imp },{ "RRA", 7, &op_rra, &am_aby },{ "NOP", 4, &op_nop, &am_abx },{ "ADC", 4, &op_adc, &am_abx },{ "ROR", 7, &op_ror, &am_abx },{ "RRA", 7, &op_rra, &am_abx },
    { "NOP", 2, &op_nop, &am_imm },{ "STA", 6, &op_sta, &am_x_ind },{ "NOP", 2, &op_nop, &am_imm },{ "SAX", 6, &op_sax, &am_x_ind },{ "STY", 3, &op_sty, &am_zpg },{ "STA", 3, &op_sta, &am_zpg },{ "STX", 3, &op_stx, &am_zpg },{ "SAX", 3, &op_sax, &am_zpg },{ "DEY", 2, &op_dey, &am_imp },{ "NOP", 2, &op_nop, &am_imm },{ "TXA", 2, &op_txa, &am_imp },{ "XAA", 2, &op_xaa, &am_imm },{ "STY", 4, &op_sty, &am_abs },{ "STA", 4, &op_sta, &am_abs },{ "STX", 4, &op_stx, &am_abs },{ "SAX", 4, &op_sax, &am_abs },
    { "BCC", 2, &op_bcc, &am_rel },{ "STA", 6, &op_sta, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "AHX", 6, &op_ahx, &am_ind_y },{ "STY", 4, &op_sty, &am_zpx },{ "STA", 4, &op_sta, &am_zpx },{ "STX", 4, &op_stx, &am_zpy },{ "SAX", 4, &op_sax, &am_zpy },{ "TYA", 2, &op_tya, &am_imp },{ "STA", 5, &op_sta, &am_aby },{ "TXS", 2, &op_txs, &am_imp },{ "TAS", 5, &op_tas, &am_aby },{ "SHY", 5, &op_shy, &am_abx },{ "STA", 5, &op_sta, &am_abx },{ "SHX", 5, &op_shx, &am_aby },{ "AHX", 5, &op_ahx, &am_aby },
    { "LDY", 2, &op_ldy, &am_imm },{ "LDA", 6, &op_lda, &am_x_ind },{ "LDX", 2, &op_ldx, &am_imm },{ "LAX", 6, &op_lax, &am_x_ind },{ "LDY", 3, &op_ldy, &am_zpg },{ "LDA", 3, &op_lda, &am_zpg },{ "LDX", 3, &op_ldx, &am_zpg },{ "LAX", 3, &op_lax, &am_zpg },{ "TAY", 2, &op_tay, &am_imp },{ "LDA", 2, &op_lda, &am_imm },{ "TAX", 2, &op_tax, &am_imp },{ "LAX", 2, &op_lax, &am_imm },{ "LDY", 4, &op_ldy, &am_abs },{ "LDA", 4, &op_lda, &am_abs },{ "LDX", 4, &op_ldx, &am_abs },{ "LAX", 4, &op_lax, &am_abs },
    { "BCS", 2, &op_bcs, &am_rel },{ "LDA", 5, &op_lda, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "LAX", 5, &op_lax, &am_ind_y },{ "LDY", 4, &op_ldy, &am_zpx },{ "LDA", 4, &op_lda, &am_zpx },{ "LDX", 4, &op_ldx, &am_zpy },{ "LAX", 4, &op_lax, &am_zpy },{ "CLV", 2, &op_clv, &am_imp },{ "LDA", 4, &op_lda, &am_aby },{ "TSX", 2, &op_tsx, &am_imp },{ "LAS", 4, &op_las, &am_aby },{ "LDY", 4, &op_ldy, &am_abx },{ "LDA", 4, &op_lda, &am_abx },{ "LDX", 4, &op_ldx, &am_aby },{ "LAX", 4, &op_lax, &am_aby },
    { "CPY", 2, &op_cpy, &am_imm },{ "CMP", 6, &op_cmp, &am_x_ind },{ "NOP", 2, &op_nop, &am_imm },{ "DCP", 8, &op_dcp, &am_x_ind },{ "CPY", 3, &op_cpy, &am_zpg },{ "CMP", 3, &op_cmp, &am_zpg },{ "DEC", 5, &op_dec, &am_zpg },{ "DCP", 5, &op_dcp, &am_zpg },{ "INY", 2, &op_iny, &am_imp },{ "CMP", 2, &op_cmp, &am_imm },{ "DEX", 2, &op_dex, &am_imp },{ "AXS", 2, &op_axs, &am_imm },{ "CPY", 4, &op_cpy, &am_abs },{ "CMP", 4, &op_cmp, &am_abs },{ "DEC", 6, &op_dec, &am_abs },{ "DCP", 6, &op_dcp, &am_abs },
    { "BNE", 2, &op_bne, &am_rel },{ "CMP", 5, &op_cmp, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "DCP", 8, &op_dcp, &am_ind_y },{ "NOP", 4, &op_nop, &am_zpx },{ "CMP", 4, &op_cmp, &am_zpx },{ "DEC", 6, &op_dec, &am_zpx },{ "DCP", 6, &op_dcp, &am_zpx },{ "CLD", 2, &op_cld, &am_imp },{ "CMP", 4, &op_cmp, &am_aby },{ "NOP", 2, &op_nop, &am_imp },{ "DCP", 7, &op_dcp, &am_aby },{ "NOP", 4, &op_nop, &am_abx },{ "CMP", 4, &op_cmp, &am_abx },{ "DEC", 7, &op_dec, &am_abx },{ "DCP", 7, &op_dcp, &am_abx },
    { "CPX", 2, &op_cpx, &am_imm },{ "SBC", 6, &op_sbc, &am_x_ind },{ "NOP", 2, &op_nop, &am_imm },{ "ISC", 8, &op_isc, &am_x_ind },{ "CPX", 3, &op_cpx, &am_zpg },{ "SBC", 3, &op_sbc, &am_zpg },{ "INC", 5, &op_inc, &am_zpg },{ "ISC", 5, &op_isc, &am_zpg },{ "INX", 2, &op_inx, &am_imp },{ "SBC", 2, &op_sbc, &am_imm },{ "NOP", 2, &op_nop, &am_imp },{ "SBC", 2, &op_sbc, &am_imm },{ "CPX", 4, &op_cpx, &am_abs },{ "SBC", 4, &op_sbc, &am_abs },{ "INC", 6, &op_inc, &am_abs },{ "ISC", 6, &op_isc, &am_abs },
    { "BEQ", 2, &op_beq, &am_rel },{ "SBC", 5, &op_sbc, &am_ind_y },{ "KIL", 2, &op_kil, &am_imp },{ "ISC", 8, &op_isc, &am_ind_y },{ "NOP", 4, &op_nop, &am_zpx },{ "SBC", 4, &op_sbc, &am_zpx },{ "INC", 6, &op_inc, &am_zpx },{ "ISC", 6, &op_isc, &am_zpx },{ "SED", 2, &op_sed, &am_imp },{ "SBC", 4, &op_sbc, &am_aby },{ "NOP", 2, &op_nop, &am_imp },{ "ISC", 7, &op_isc, &am_aby },{ "NOP", 4, &op_nop, &am_abx },{ "SBC", 4, &op_sbc, &am_abx },{ "INC", 7, &op_inc, &am_abx },{ "ISC", 7, &op_isc, &am_abx }
};


uint8_t get_flag(cpu_t *cpu, status_mask_t flag) {
    return (cpu->status & flag) != 0;
}

void set_flag(cpu_t *cpu, status_mask_t flag, bool value) {
    cpu->status = value ? cpu->status | flag : cpu->status & (~flag);
}

void cpu_reset(cpu_t *cpu) {
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;

    /* the 6502 actually just accesses the stack 3 times during the reset
     * procedure, it doesn't set the sp to a known value, I am doing so here as
     * a convenience, mainly for testing. Perhaps I should mimic the actual
     * behavior of the 6502
     * see http://users.telenet.be/kim1-6502/6502/proman.html#92 */
    cpu->sp = 0xFD; 
       
    cpu->status = STATUS_I | STATUS_U;

    /* TODO(shaw): the reset vector is 0xFFFC, 0xFFFD on startup the cpu would
     * read the values at these locations into pc and perform a JMP. For now
     * i'm just hardcoding the pc to a value */
    /*cpu->pc = 0xC000;*/

    cpu->pc = bus_read(0xFFFC) | (bus_read(0xFFFD) << 8);

    cpu->cycles = 0;
    cpu->op_cycles = 7; /* reset takes 7 cycles */

    cpu->running = true;
}

dasm_map_t *disassemble(uint16_t start, uint16_t stop) {
    dasm_map_t *dasm = NULL;
    hmdefault(dasm, "");

    uint8_t opcode;
    uint16_t addr = start;
    uint8_t (*am)(cpu_t *cpu, uint16_t *addr);

    while (addr >= start && addr <= stop) {
        char *ins = malloc(24*sizeof(char));
        if (!ins) { perror("malloc"); exit(1); }
        char am_string[13] = {0};
        opcode = bus_read(addr);
        op_t op = ops[opcode];
        am = op.addr_mode;
        int addr_inc = 0;

        if (am == am_imp) {
            if (opcode == 0x0A || opcode == 0x2A || opcode == 0x4A || opcode == 0x6A) {
                snprintf(am_string, 13, "A"); 
            }
            addr_inc = 1;

        } else if (am == am_abs) {
            snprintf(am_string, 13, "$%.2X%.2X,X", bus_read(addr+2), bus_read(addr+1));
            addr_inc = 3;

        } else if (am == am_abx) {
            snprintf(am_string, 13, "$%.2X%.2X,X", bus_read(addr+2), bus_read(addr+1));
            addr_inc = 3;

        } else if (am == am_aby) {
            snprintf(am_string, 13, "$%.2X%.2X,Y", bus_read(addr+2), bus_read(addr+1));
            addr_inc = 3;

        } else if (am == am_imm) {
            snprintf(am_string, 13, "#$%.2X", bus_read(addr+1));
            addr_inc = 2;
            
        } else if (am == am_ind) {
            snprintf(am_string, 13, "($%.2X%.2X)", bus_read(addr+2), bus_read(addr+1));
            addr_inc = 3;
            
        } else if (am == am_x_ind) {
            snprintf(am_string, 13, "($%.2X,X)", bus_read(addr+1));
            addr_inc = 2;
            
        } else if (am == am_ind_y) {
            snprintf(am_string, 13, "($%.2X),Y", bus_read(addr+1));
            addr_inc = 2;
            
        } else if (am == am_rel) {
            snprintf(am_string, 13, "$%.2X", bus_read(addr+1));
            addr_inc = 2;
            
        } else if (am == am_zpg) {
            snprintf(am_string, 13, "$%.2X", bus_read(addr+1));
            addr_inc = 2;
            
        } else if (am == am_zpx) {
            snprintf(am_string, 13, "$%.2X,X", bus_read(addr+1));
            addr_inc = 2;
            
        } else if (am == am_zpy) {
            snprintf(am_string, 13, "$%.2X,Y", bus_read(addr+1));
            addr_inc = 2;
        }

        snprintf(ins, 24, "$%.4X: %3s %s", addr, op.name, am_string);

        hmput(dasm, addr, ins);
        addr += addr_inc;
    }

    return dasm;
}


#ifdef DEBUG_LOG
uint16_t ppu_get_cycle(void);
uint16_t ppu_get_scanline(void);
void debug_log_instruction(cpu_t *cpu) {
    assert(logfile && "logfile is not defined or could not be opened");

    char operands[6] = {0}; 
    char decoded[27] = {0};
    op_t op = ops[cpu->opcode];

    uint8_t (*am)(cpu_t *cpu, uint16_t *addr) = op.addr_mode;
    uint8_t (*exe)(cpu_t *cpu, uint16_t addr) = op.execute;

    if (am == am_imp) {
        switch (cpu->opcode) {
            case 0x0A:
            case 0x2A:
            case 0x4A:
            case 0x6A: decoded[0] = 'A'; break;
            default: break;
        }

    } else if (am == am_abs) {
        word_t operand;
        operand.byte.l = bus_read(cpu->pc);
        operand.byte.h = bus_read(cpu->pc+1);
        snprintf(operands, 6, "%.2X %.2X", operand.byte.l, operand.byte.h);
        if (exe == op_jmp || exe == op_jsr)
            snprintf(decoded, 27, "$%.4X", operand.w);
        else
            snprintf(decoded, 27, "$%.4X = %.2X", operand.w, bus_read(operand.w));

    } else if (am == am_abx) {
        word_t operand, addr;
        operand.byte.l = bus_read(cpu->pc);
        operand.byte.h = bus_read(cpu->pc+1);
        addr.w = operand.w + cpu->x;
        snprintf(operands, 6, "%.2X %.2X", operand.byte.l, operand.byte.h);
        snprintf(decoded, 27, "$%.4X,X @ %.4X = %.2X", operand.w, addr.w, bus_read(addr.w));

    } else if (am == am_aby) {
        word_t operand, addr;
        operand.byte.l = bus_read(cpu->pc);
        operand.byte.h = bus_read(cpu->pc+1);
        addr.w = operand.w + cpu->y;
        snprintf(operands, 6, "%.2X %.2X", operand.byte.l, operand.byte.h);
        snprintf(decoded, 27, "$%.4X,Y @ %.4X = %.2X", operand.w, addr.w, bus_read(addr.w));

    } else if (am == am_imm) {
        uint8_t operand = bus_read(cpu->pc);
        snprintf(operands, 6, "%.2X", operand);
        snprintf(decoded, 27, "#$%.2X", operand);
        
    } else if (am == am_ind) {
        word_t pointer, addr;
        pointer.byte.l = bus_read(cpu->pc);
        pointer.byte.h = bus_read(cpu->pc+1);
        addr.byte.l = bus_read(pointer.w);
        uint16_t ptr_high_byte = pointer.byte.l == 0xFF ? pointer.w & 0xFF00 : pointer.w + 1;
        addr.byte.h = bus_read(ptr_high_byte);
        snprintf(operands, 6, "%.2X %.2X", pointer.byte.l, pointer.byte.h);
        snprintf(decoded, 27, "($%.4X) = %.4X", pointer.w, addr.w);
        
    } else if (am == am_x_ind) {
        uint8_t operand = bus_read(cpu->pc);
        snprintf(operands, 6, "%.2X", operand);
        uint8_t zpg = cpu->x + operand;
        word_t pointer;
        pointer.byte.l = bus_read(zpg);
        pointer.byte.h = bus_read((uint8_t)(zpg+1));
        snprintf(decoded, 27, "($%.2X,X) @ %.2X = %.4X = %.2X", operand, zpg, pointer.w, bus_read(pointer.w));
        
    } else if (am == am_ind_y) {
        uint8_t zpg = bus_read(cpu->pc);
        word_t pointer;
        pointer.byte.l = bus_read(zpg);
        pointer.byte.h = bus_read((uint8_t)(zpg+1));
        uint16_t addr = pointer.w + cpu->y;
        snprintf(operands, 6, "%.2X", zpg);
        snprintf(decoded, 27, "($%.2X),Y = %.4X @ %.4X = %.2X", zpg, pointer.w, addr, bus_read(addr));
        
    } else if (am == am_rel) {
        int8_t offset = bus_read(cpu->pc);
        uint16_t addr = offset + cpu->pc + 1;
        snprintf(operands, 6, "%.2X", (uint8_t)offset);
        snprintf(decoded, 27, "$%.4X", addr);
        
    } else if (am == am_zpg) {
        uint8_t operand = bus_read(cpu->pc);
        snprintf(operands, 6, "%.2X", operand);
        snprintf(decoded, 27, "$%.2X = %.2X", operand, bus_read(operand));
        
    } else if (am == am_zpx) {
        uint8_t operand = bus_read(cpu->pc);
        uint8_t addr = cpu->x + operand;
        snprintf(operands, 6, "%.2X", operand);
        snprintf(decoded, 27, "$%.2X,X @ %.2X = %.2X", operand, addr, bus_read(addr));
        
    } else if (am == am_zpy) {
        uint8_t operand = bus_read(cpu->pc);
        uint8_t addr = cpu->y + operand;
        snprintf(operands, 6, "%.2X", operand);
        snprintf(decoded, 27, "$%.2X,Y @ %.2X = %.2X", operand, addr, bus_read(addr));
    }

    fprintf(logfile, "%.4X  %.2X %-5s  %3s %-26s  A:%.2X X:%.2X Y:%.2X P:%.2X SP:%.2X PPU:%3d,%3d CYC:%ld\n", cpu->pc-1, cpu->opcode, operands, op.name, decoded, cpu->a, cpu->x, cpu->y, cpu->status, cpu->sp, ppu_get_scanline(), ppu_get_cycle(), cpu->cycles);
}
#endif

void print_message_at(uint16_t addr) {
    uint8_t buf[256] = {0};
    int i = 0;
    uint8_t data;
    do {
        data = bus_read(addr++);
        buf[i++] = data;
    } while(data);

    printf("Message:\n%s\n", buf);
}

void print_cpu_state(cpu_t *cpu) {
    printf("PC\t\tA\t\tX\t\tY\t\tSP\t\tN V _ B D I Z C\n"); 
    printf("0x%.4X\t\t0x%.2X\t\t0x%.2X\t\t0x%.2X\t\t0x%2X\t\t%d %d %d %d %d %d %d %d\n", 
        cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, (cpu->status >> 7) & 1, 
        (cpu->status >> 6) & 1, (cpu->status >> 5) & 1, (cpu->status >> 4) & 1, 
        (cpu->status >> 3) & 1, (cpu->status >> 2) & 1, (cpu->status >> 1) & 1, 
        (cpu->status >> 0) & 1);
}


void cpu_tick(cpu_t *cpu) {
    if (cpu->op_cycles == 0) {
        cpu->opcode = bus_read(cpu->pc++);
        op_t op = ops[cpu->opcode];
        cpu->op_cycles = op.cycles;

#ifdef DEBUG_LOG
        debug_log_instruction(cpu);
#endif

        uint16_t addr; 
        uint8_t am_add_cycle = op.addr_mode(cpu, &addr);
        uint8_t op_add_cycle = op.execute(cpu, addr);

        cpu->op_cycles += (am_add_cycle & op_add_cycle);
    }
    --cpu->op_cycles;
    ++cpu->cycles;
}


