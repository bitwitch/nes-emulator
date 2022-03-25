
case 0x00: /* BRK impl */
    exit(1); /* TEMPORARY */
    break;
case 0x01: printf("ORA X, ind\n"); break;
case 0x05: printf("ORA zpg\n"); break;
case 0x06: printf("ASL zpg\n"); break;
case 0x08: printf("PHP imp\n"); break;
case 0x09: printf("ORA #\n"); break;
case 0x0A: printf("ASL A\n"); break;
case 0x0D: printf("ORA abs\n"); break;
case 0x0E: printf("ASL abs\n"); break;

case 0x10: printf("BPL rel\n"); break;
case 0x11: printf("ORA ind, Y\n"); break;
case 0x15: printf("ORA zpg, X\n"); break;
case 0x16: printf("ASL zpg, X\n"); break;
case 0x18: printf("CLC imp\n"); break;
case 0x19: printf("ORA abs, Y\n"); break;
case 0x1D: printf("ORA abs, X\n"); break;
case 0x1E: printf("ASL abs, X\n"); break;

case 0x20: printf("JSR abs\n"); break;
case 0x21: printf("AND X, ind\n"); break;
case 0x24: printf("BIT zpg\n"); break;
case 0x25: printf("AND zpg\n"); break;
case 0x26: printf("ROL zpg\n"); break;
case 0x28: printf("PLP impl\n"); break;
case 0x29: printf("AND #\n"); break;
case 0x2A: printf("ROL A\n"); break;
case 0x2C: printf("BIT abs\n"); break;
case 0x2D: printf("AND abs\n"); break;
case 0x2E: printf("ROL abs\n"); break;

case 0x30: printf("BMI rel\n"); break;
case 0x31: printf("AND ind, Y\n"); break;
case 0x35: printf("AND zpg, X\n"); break;
case 0x36: printf("ROL zpg, X\n"); break;
case 0x38: printf("SEC imp\n"); break;
case 0x39: printf("AND abs, Y\n"); break;
case 0x3D: printf("AND abs, X\n"); break;
case 0x3E: printf("ROL abs, X\n"); break;

case 0x40: printf("RTI imp\n"); break;
case 0x41: printf("EOR X, ind\n"); break;
case 0x45: printf("EOR zpg\n"); break;
case 0x46: printf("LSR zpg\n"); break;
case 0x48: printf("PHA imp\n"); break;
case 0x49: printf("EOR #\n"); break;
case 0x4A: printf("LSR A\n"); break;
case 0x4C: printf("JMP abs\n"); break;
case 0x4D: printf("EOR abs\n"); break;
case 0x4E: printf("LSR abs\n"); break;

case 0x50: printf("BVC rel\n"); break;
case 0x51: printf("EOR ind, Y\n"); break;
case 0x55: printf("EOR zpg, X\n"); break;
case 0x56: printf("LSR zpg, X\n"); break;
case 0x58: printf("CLI imp\n"); break;
case 0x59: printf("EOR abs, Y\n"); break;
case 0x5D: printf("EOR abs, X\n"); break;
case 0x5E: printf("LSR abs, X\n"); break;

case 0x60: printf("RTS imp\n"); break;
case 0x61: printf("ADC X, ind\n"); break;
case 0x65: printf("ADC zpg\n"); break;
case 0x66: printf("ROR zpg\n"); break;
case 0x68: printf("PLA imp\n"); break;
case 0x69: /* ADC # */
    cpu->a_reg += read_addr(cpu->pc++);
    break;
case 0x6A: printf("ROR A\n"); break;
case 0x6C: printf("JMP ind\n"); break;
case 0x6D: printf("ADC abs\n"); break;
case 0x6E: printf("ROR abs\n"); break;

case 0x70: printf("BVS rel\n"); break;
case 0x71: printf("ADC ind, Y\n"); break;
case 0x75: printf("ADC zpg, X\n"); break;
case 0x76: printf("ROR zpg, X\n"); break;
case 0x78: printf("SEI imp\n"); break;
case 0x79: printf("ADC abs, Y\n"); break;
case 0x7D: printf("ADC abs, X\n"); break;
case 0x7E: printf("ROR abs, X\n"); break;

case 0x81: printf("STA X, ind\n"); break;
case 0x84: printf("STY zpg\n"); break;
case 0x85: printf("STA zpg\n"); break;
case 0x86: printf("STX zpg\n"); break;
case 0x88: printf("DEY imp\n"); break;
case 0x8A: printf("TXA imp\n"); break;
case 0x8C: printf("STY abs\n"); break;
case 0x8D: printf("STA abs\n"); break;
case 0x8E: printf("STX abs\n"); break;

case 0x90: printf("BCC rel\n"); break;
case 0x91: printf("STA ind, Y\n"); break;
case 0x94: printf("STY zpg, X\n"); break;
case 0x95: printf("STA zpg, X\n"); break;
case 0x96: printf("STX zpg, Y\n"); break;
case 0x98: printf("TYA imp\n"); break;
case 0x99: printf("STA abs, Y\n"); break;
case 0x9A: printf("TXS imp\n"); break;
case 0x9D: printf("STA abs, X\n"); break;

case 0xA0: printf("LDY #\n"); break;
case 0xA1: printf("LDA X, ind\n"); break;
case 0xA2: printf("LDX #\n"); break;
case 0xA4: printf("LDY zpg\n"); break;
case 0xA5: printf("LDA zpg\n"); break;
case 0xA6: printf("LDX zpg\n"); break;
case 0xA8: printf("TAY imp\n"); break;
case 0xA9: printf("LDA #\n"); break;
case 0xAA: printf("TAX imp\n"); break;
case 0xAC: printf("LDY abs\n"); break;
case 0xAD: printf("LDA abs\n"); break;
case 0xAE: printf("LDX abs\n"); break;

case 0xB0: printf("BCS rel\n"); break;
case 0xB1: printf("LDA ind, Y\n"); break;
case 0xB4: printf("LDY zpg, X\n"); break;
case 0xB5: printf("LDA zpg, X\n"); break;
case 0xB6: printf("LDX zpg, Y\n"); break;
case 0xB8: printf("CLV imp\n"); break;
case 0xB9: printf("LDA abs, Y\n"); break;
case 0xBA: printf("TSX imp\n"); break;
case 0xBC: printf("LDY abs, X\n"); break;
case 0xBD: printf("LDA abs, X\n"); break;
case 0xBE: printf("LDX abs, Y\n"); break;

case 0xC0: printf("CPY #\n"); break;
case 0xC1: printf("CMP X, ind\n"); break;
case 0xC4: printf("CPY zpg\n"); break;
case 0xC5: printf("CMP zpg\n"); break;
case 0xC6: printf("DEC zpg\n"); break;
case 0xC8: printf("INY imp\n"); break;
case 0xC9: printf("CMP #\n"); break;
case 0xCA: printf("DEX imp\n"); break;
case 0xCC: printf("CPY abs\n"); break;
case 0xCD: printf("CMP abs\n"); break;
case 0xCE: printf("DEC abs\n"); break;

case 0xD0: printf("BNE rel\n"); break;
case 0xD1: printf("CMP ind, Y\n"); break;
case 0xD5: printf("CMP zpg, X\n"); break;
case 0xD6: printf("DEC zpg, X\n"); break;
case 0xD8: printf("CLD imp\n"); break;
case 0xD9: printf("CMP abs, Y\n"); break;
case 0xDD: printf("CMP abs, X\n"); break;
case 0xDE: printf("DEC abs, X\n"); break;

case 0xE0: printf("CPX #\n"); break;
case 0xE1: printf("SBC X, ind\n"); break;
case 0xE4: printf("CPX zpg\n"); break;
case 0xE5: printf("SBC zpg\n"); break;
case 0xE6: printf("INC zpg\n"); break;
case 0xE8: printf("INX imp\n"); break;
case 0xE9: printf("SBC #\n"); break;
case 0xEA: printf("NOP imp\n"); break;
case 0xEC: printf("CPX abs\n"); break;
case 0xED: printf("SBC abs\n"); break;
case 0xEE: printf("INC abs\n"); break;

case 0xF0: printf("BEQ rel\n"); break;
case 0xF1: printf("SBC ind, Y\n"); break;
case 0xF5: printf("SBC zpg, X\n"); break;
case 0xF6: printf("INC zpg, X\n"); break;
case 0xF8: printf("SED imp\n"); break;
case 0xF9: printf("SBC abs, Y\n"); break;
case 0xFD: printf("SBC abs, X\n"); break;
case 0xFE: printf("INC abs, X\n"); break;

default: printf("illegal opcode 0x%.2X\n", opcode); break;





