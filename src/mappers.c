/*
 * Each mapper must implement:
 *     uint32_t mapperXX_read(mapper_t *head, uint16_t addr)
 *     uint32_t mapperXX_write(mapper_t *head, uint16_t addr, uint8_t data)
 *
 *  These functions can do any internal operation the mapper needs and must
 *  return the translated address to the actual location on the cartridge
 *
 *
 * See "How I Program C by Eskil Steenberg" for more info on the polymorphism
 * pattern used in this file by mappers
 * https://www.youtube.com/watch?v=443UNeGrFoM
 *
 */

#define _KB       1024
#define _4KB      4096
#define _8KB      8192
#define _16KB     16384
#define _32KB     32768

typedef enum {
	MIRROR_HORIZONTAL,
	MIRROR_VERTICAL,
} MirrorMode;

typedef struct {
    uint8_t prg_banks;    /* number of 16KB prg rom banks */
    uint8_t chr_banks;    /* number of  8KB chr rom banks */
    MirrorMode mirroring; /* 0 == horizontal 1 == vertical */
    uint16_t id;          /* mapper number */
} mapper_t;


/***************************************************************************** 
 * MAPPER 0 
 ****************************************************************************/
typedef struct {
    mapper_t head;
} mapper0_t;

static uint32_t
mapper0_map_addr(mapper_t *head, uint16_t addr) {
    if (addr < 0x2000) 
        return addr;

    /* horizontal and vertical nametable mirroring */
    else if (addr < 0x3F00) {
        uint32_t mapped_addr = addr & 0x0FFF;
        if (head->mirroring == MIRROR_VERTICAL)
            return mapped_addr & 0x07FF;
        else {
            return (mapped_addr < 0x0800)
                ? (mapped_addr & 0x03FF)
                : ((mapped_addr-0x0800) & 0x03FF) + 0x0400;
        }
    } 

    else if (addr < 0x6000) 
        return addr;
    else if (addr < 0x8000) /* $6000 - $7FFF */
        return addr - 0x6000;
    else if (addr < 0xC000) /* $8000 - $BFFF */
        return addr - 0x8000;
    else                    /* $C000 - $FFFF */
        /* mirror the first 16KB of prg_rom if NROM-128 */
        return head->prg_banks == 1 ? addr - 0xC000 : addr - 0x8000;
}

static uint32_t 
mapper0_read(mapper_t *head, uint16_t addr) {
    return mapper0_map_addr(head, addr);
}

static uint32_t 
mapper0_write(mapper_t *head, uint16_t addr, uint8_t data) {
    (void)data;
    return mapper0_map_addr(head, addr);
}

/***************************************************************************** 
 * MAPPER 1 
 ****************************************************************************/
typedef struct {
    mapper_t head;
    uint8_t shift_reg;
    uint8_t control; 
    uint8_t chr_bank0;
    uint8_t chr_bank1;
    uint8_t prg_bank;
} mapper1_t;

static uint32_t
mapper1_map_addr(mapper_t *head, uint16_t addr) {
    mapper1_t *mapper = (mapper1_t *)head;

    if (addr < 0x2000) { 
        // CHR ROM bank mode
        if ((mapper->control >> 4) & 1) {
        //switch two separate 4KB banks
            if (addr < 0x1000)
                return addr + mapper->chr_bank0 * _4KB;
            else 
                return (addr - 0x1000) + mapper->chr_bank1 * _4KB; // TODO(shaw): make sure this is correct
        } else {
        //switch 8KB at a time
            return addr + mapper->chr_bank0 * _8KB;
        }
    }

    // nametable mirroring
    else if (addr < 0x3F00) {
        uint32_t mapped_addr = addr & 0x0FFF;
        uint8_t mirror_mode = mapper->control & 0x3;

        switch (mirror_mode) {
        case 0: // one-screen, lower bank
            return mapped_addr & 0x03FF;
        case 1: // one-screen, upper bank
            return (mapped_addr & 0x03FF) + 0x0400;
        case 2: // vertical
            return mapped_addr & 0x07FF;
        case 3: // horizontal
            return mapped_addr < 0x0800
                ? mapped_addr & 0x03FF
                : ((mapped_addr-0x0800) & 0x03FF) + 0x0400;
        default:
            assert(0 && "unknown mirror mode");
			return 0;
        }
    } 

    else if (addr < 0x6000) 
        return addr;
    else if (addr < 0x8000)   /* $6000 - $7FFF */
        return addr - 0x6000;

    // first 16KB bank (or 32KB if in that mode)
    else if (addr < 0xC000) { /* $8000 - $BFFF */
        uint8_t prg_bank_mode = (mapper->control >> 2) & 0x3;
        switch (prg_bank_mode) {
            case 0:
            case 1: // switch 32 KB at $8000, ignoring low bit of bank number
            {
                uint8_t prg_bank = mapper->prg_bank & 0x0F;
                return (addr - 0x8000) + (prg_bank * _32KB);
            }
            case 2: // fix first bank at $8000 
                return (addr - 0x8000);
            case 3: // switch 16 KB bank at $8000
            {
                uint8_t prg_bank = mapper->prg_bank & 0x0F;
                return (addr - 0x8000) + (prg_bank * _16KB);
            }
            default:
                assert(0 && "unknown prg_bank_mode");
                return 0;
        }
    }

    // second 16KB bank (or 32KB if in that mode)
    else { /* $C000 - $FFFF */
        uint8_t prg_bank_mode = (mapper->control >> 2) & 0x3;
        switch (prg_bank_mode) {
            case 0:
            case 1: // switch 32 KB at $8000, ignoring low bit of bank number
            {
                uint8_t prg_bank = mapper->prg_bank & 0x0F;
                return (addr - 0x8000) + (prg_bank * _32KB);
            }
            case 2: // switch 16 KB bank at $C000
            {
                uint8_t prg_bank = mapper->prg_bank & 0x0F;
                return (addr - 0xC000) + (prg_bank * _16KB);
            }
            case 3: // fix last bank at $C000
                return (addr - 0xC000) + ((head->prg_banks-1) * _16KB);
            default:
                assert(0 && "unknown prg_bank_mode");
				return 0;
        }
    }
}

static uint32_t 
mapper1_read(mapper_t *head, uint16_t addr) {
    return mapper1_map_addr(head, addr);
}

static uint32_t 
mapper1_write(mapper_t *head, uint16_t addr, uint8_t data) {
    static uint8_t shift_count = 0;
    if (addr >= 0x8000) {
        mapper1_t *mapper = (mapper1_t *)head;

        if ((data >> 7) & 1) {
            // clear shift register to its initial state
            mapper->shift_reg = 0;
            shift_count = 0;

            // set 16k PRG mode, $8000 swappable
            // NOTE(shaw): Disch includes this in his mapper notes, but I can't
            // find it on the nesdev wiki anywhere. I am inclined to believe
            // Disch though.
            // https://www.romhacking.net/download/documents/362/
            mapper->control |= 0xC;
        } else {
            // shift bit 0 of data into shift register
            mapper->shift_reg = (mapper->shift_reg & 0x1F) | ((data & 1) << 5);
            mapper->shift_reg >>= 1;
            ++shift_count;

            // on fifth write, copy to an internal register
            if (shift_count == 5) {

                if (addr < 0xA000) {         // $8000 - $9FFF
                    mapper->control = mapper->shift_reg;
                } else if (addr < 0xC000) {  // $A000 - $BFFF
                    mapper->chr_bank0 = mapper->shift_reg;
                } else if (addr < 0xE000) {  // $C000 - $DFFF
                    mapper->chr_bank1 = mapper->shift_reg;
                } else {                     // $E000 - $FFFF
                    mapper->prg_bank = mapper->shift_reg;
                }

                mapper->shift_reg = 0;
                shift_count = 0;
            }
        }
    }

    return mapper1_map_addr(head, addr);
}




/***************************************************************************** 
 * MAPPER 2 
 ****************************************************************************/
typedef struct {
    mapper_t head;
    uint8_t prg_bank; // mapper2 has up to 4096K of bank switchable prg rom
} mapper2_t;

static uint32_t
mapper2_map_addr(mapper_t *head, uint16_t addr) {
    mapper2_t *mapper = (mapper2_t *)head;

    if (addr < 0x2000) 
        return addr;

    /* horizontal and vertical nametable mirroring */
    else if (addr < 0x3F00) {
        uint32_t mapped_addr = addr & 0x0FFF;
        if (head->mirroring == MIRROR_VERTICAL)
            return mapped_addr & 0x07FF;
        else
            return mapped_addr < 0x0800
                ? mapped_addr & 0x03FF
                : ((mapped_addr-0x0800) & 0x03FF) + 0x0400;
    } 

    else if (addr < 0x6000) 
        return addr;
    else if (addr < 0x8000) /* $6000 - $7FFF */
        return addr - 0x6000;
    else if (addr < 0xC000) /* $8000 - $BFFF */
        return (addr - 0x8000) + (mapper->prg_bank * _16KB);
    else                    /* $C000 - $FFFF */
        /* fixed to the last bank */
        return (addr - 0xC000) + ((head->prg_banks-1) * _16KB);
}

static uint32_t 
mapper2_read(mapper_t *head, uint16_t addr) {
    return mapper2_map_addr(head, addr);
}

static uint32_t 
mapper2_write(mapper_t *head, uint16_t addr, uint8_t data) {
    if (addr >= 0x8000) {
        mapper2_t *mapper = (mapper2_t *)head;
        mapper->prg_bank = data;
    }
    return mapper2_map_addr(head, addr);
}



/***************************************************************************** 
 * MAPPER 3 
 ****************************************************************************/
typedef struct {
    mapper_t head;
    uint8_t chr_bank; // mapper 3 has up to 2048K of bank switchable CHR rom
} mapper3_t;

static uint32_t
mapper3_map_addr(mapper_t *head, uint16_t addr) {
    mapper3_t *mapper = (mapper3_t *)head;

    if (addr < 0x2000) {
        // ppu bank switchable chr rom
        // NOTE(shaw): banks not used by the cartridge are mirrored over banks
        // that are used
        return addr + ((mapper->chr_bank % head->chr_banks) * _8KB);
    }

    /* horizontal and vertical nametable mirroring */
    else if (addr < 0x3F00) {
        uint32_t mapped_addr = addr & 0x0FFF;
        if (head->mirroring == MIRROR_VERTICAL)
            return mapped_addr & 0x07FF;
        else
            return mapped_addr < 0x0800
                ? mapped_addr & 0x03FF
                : ((mapped_addr-0x0800) & 0x03FF) + 0x0400;
        return addr;
    } 

    else if (addr < 0x6000) 
        return addr;
    else if (addr < 0x8000) /* $6000 - $7FFF */
        return addr - 0x6000;
    else if (addr < 0xC000) /* $8000 - $BFFF */
        return (addr - 0x8000);
    else                    /* $C000 - $FFFF */
        /* mirror the first 16KB of prg_rom if PRG ROM capacity is 16KB */
        return head->prg_banks == 1 ? addr - 0xC000 : addr - 0x8000;
}

static uint32_t 
mapper3_read(mapper_t *head, uint16_t addr) {
    return mapper3_map_addr(head, addr);
}

static uint32_t 
mapper3_write(mapper_t *head, uint16_t addr, uint8_t data) {
    if (addr >= 0x8000) {
        mapper3_t *mapper = (mapper3_t *)head;
        mapper->chr_bank = data;
    }
    return mapper3_map_addr(head, addr);
}


/***************************************************************************** 
 * MAPPER 4
 ****************************************************************************/
/*
 *  CPU $6000-$7FFF: 8 KB PRG RAM bank (optional)
 *  CPU $8000-$9FFF (or $C000-$DFFF): 8 KB switchable PRG ROM bank
 *  CPU $A000-$BFFF: 8 KB switchable PRG ROM bank
 *  CPU $C000-$DFFF (or $8000-$9FFF): 8 KB PRG ROM bank, fixed to the second-last bank
 *  CPU $E000-$FFFF: 8 KB PRG ROM bank, fixed to the last bank
 *  PPU $0000-$07FF (or $1000-$17FF): 2 KB switchable CHR bank
 *  PPU $0800-$0FFF (or $1800-$1FFF): 2 KB switchable CHR bank
 *  PPU $1000-$13FF (or $0000-$03FF): 1 KB switchable CHR bank
 *  PPU $1400-$17FF (or $0400-$07FF): 1 KB switchable CHR bank
 *  PPU $1800-$1BFF (or $0800-$0BFF): 1 KB switchable CHR bank
 *  PPU $1C00-$1FFF (or $0C00-$0FFF): 1 KB switchable CHR bank
 */
typedef struct {
    mapper_t head;
    uint8_t bank_select; 
	uint8_t bank_regs[8];
	uint32_t prg_bank[4];
	uint32_t chr_bank[8];
	uint8_t prg_bank_mode;
	uint8_t chr_inversion;
	uint8_t irq_counter;
	uint8_t irq_load;
	bool irq_enabled;
	bool irq_pending;

	// NOTE: Though these bits are functional on the MMC3, their main purpose
	// is to write-protect save RAM during power-off. Many emulators choose not
	// to implement them as part of iNES Mapper 4 to avoid an incompatibility
	// with the MMC6.
	bool write_protect;
	bool prg_ram_enable;
} mapper4_t;

static uint32_t
mapper4_map_addr(mapper_t *head, uint16_t addr) {
    mapper4_t *mapper = (mapper4_t *)head;

	// CHR Banks
	if (addr < 0x0400)                               // $0000-$03FF
		return mapper->chr_bank[0] + (addr & 0x03FF);
	else if (addr < 0x0800)                          // $0400-$07FF
		return mapper->chr_bank[1] + (addr & 0x03FF);
	else if (addr < 0x0C00)                          // $0800-$0BFF
		return mapper->chr_bank[2] + (addr & 0x03FF);
	else if (addr < 0x1000)                          // $0C00-$0FFF
		return mapper->chr_bank[3] + (addr & 0x03FF);
	else if (addr < 0x1400)                          // $1000-$13FF
		return mapper->chr_bank[4] + (addr & 0x03FF);
	else if (addr < 0x1800)                          // $1400-$17FF
		return mapper->chr_bank[5] + (addr & 0x03FF);
	else if (addr < 0x1C00)                          // $1800-$1BFF
		return mapper->chr_bank[6] + (addr & 0x03FF);
	else if (addr < 0x2000)                          // $1C00-$1FFF
		return mapper->chr_bank[7] + (addr & 0x03FF);

    /* horizontal and vertical nametable mirroring */
	else if (addr < 0x3F00) {
        uint32_t mapped_addr = addr & 0x0FFF;
        if (head->mirroring == MIRROR_VERTICAL)
            return mapped_addr & 0x07FF;
        else
            return mapped_addr < 0x0800
                ? mapped_addr & 0x03FF
                : ((mapped_addr-0x0800) & 0x03FF) + 0x0400;
    } 

    else if (addr < 0x6000) 
        return addr;
    else if (addr < 0x8000) // $6000-$7FFF 
        return addr & 0x1FFF;

	// PRG Banks
    else if (addr < 0xA000)                          // $8000-$9FFF
		return mapper->prg_bank[0] + (addr & 0x1FFF);
    else if (addr < 0xC000)                          // $A000-$BFFF
		return mapper->prg_bank[1] + (addr & 0x1FFF);
    else if (addr < 0xE000)                          // $C000-$DFFF
		return mapper->prg_bank[2] + (addr & 0x1FFF);
    else                                             // $E000-$FFFF
		return mapper->prg_bank[3] + (addr & 0x1FFF);
}

static uint32_t 
mapper4_read(mapper_t *head, uint16_t addr) {
    return mapper4_map_addr(head, addr);
}

static uint32_t 
mapper4_write(mapper_t *head, uint16_t addr, uint8_t data) {
	mapper4_t *mapper = (mapper4_t *)head;

	if (addr < 0x8000) {
		// do nothing
	} else if (addr < 0xA000) { // $8000-$9FFF
		if (addr & 1) { // odd
			mapper->bank_regs[mapper->bank_select] = data;

			if (mapper->chr_inversion) {
				mapper->chr_bank[0] = mapper->bank_regs[2] * _KB;
				mapper->chr_bank[1] = mapper->bank_regs[3] * _KB;
				mapper->chr_bank[2] = mapper->bank_regs[4] * _KB;
				mapper->chr_bank[3] = mapper->bank_regs[5] * _KB;
				mapper->chr_bank[4] = (mapper->bank_regs[0] & 0xFE) * _KB;
				mapper->chr_bank[5] = (mapper->bank_regs[0] + 1)    * _KB;
				mapper->chr_bank[6] = (mapper->bank_regs[1] & 0xFE) * _KB;
				mapper->chr_bank[7] = (mapper->bank_regs[1] + 1)    * _KB;
			} else {
				mapper->chr_bank[0] = (mapper->bank_regs[0] & 0xFE) * _KB;
				mapper->chr_bank[1] = (mapper->bank_regs[0] + 1)    * _KB;
				mapper->chr_bank[2] = (mapper->bank_regs[1] & 0xFE) * _KB;
				mapper->chr_bank[3] = (mapper->bank_regs[1] + 1)    * _KB;
				mapper->chr_bank[4] = mapper->bank_regs[2] * _KB;
				mapper->chr_bank[5] = mapper->bank_regs[3] * _KB;
				mapper->chr_bank[6] = mapper->bank_regs[4] * _KB;
				mapper->chr_bank[7] = mapper->bank_regs[5] * _KB;
			}

			// last and second last prg banks (in 8KB chunks)
			uint8_t last = head->prg_banks*2 - 1;
			uint8_t second_last = last - 1;

			if (mapper->prg_bank_mode) {
				mapper->prg_bank[0] = second_last * _8KB;
				mapper->prg_bank[1] = (mapper->bank_regs[7] & 0x3F) * _8KB;
				mapper->prg_bank[2] = (mapper->bank_regs[6] & 0x3F) * _8KB;
				mapper->prg_bank[3] = last * _8KB;
			} else {
				mapper->prg_bank[0] = (mapper->bank_regs[6] & 0x3F) * _8KB;
				mapper->prg_bank[1] = (mapper->bank_regs[7] & 0x3F) * _8KB;
				mapper->prg_bank[2] = second_last * _8KB;
				mapper->prg_bank[3] = last * _8KB;
			}
		} else { // even
			mapper->bank_select = data & 0x7;
			mapper->prg_bank_mode = (data >> 6) & 1;
			mapper->chr_inversion = (data >> 7) & 1;
		}

	} else if (addr < 0xC000) { // $A000-$BFFF
		if (addr & 1) { // odd
			mapper->write_protect  = (data >> 6) & 1;
			mapper->prg_ram_enable = (data >> 7) & 1;
			// NOTE: Though these bits are functional on the MMC3, their main
			// purpose is to write-protect save RAM during power-off. Many
			// emulators choose not to implement them as part of iNES Mapper 4
			// to avoid an incompatibility with the MMC6.

		} else { //even
			head->mirroring = (data & 1) ? MIRROR_HORIZONTAL : MIRROR_VERTICAL;
			// NOTE: This bit has no effect on cartridges with hardwired
			// 4-screen VRAM. In the iNES and NES 2.0 formats, this can be
			// identified through bit 3 of byte $06 of the header.
		}

	} else if (addr < 0xE000) { // $C000-$DFFF
		if (addr & 1) { // odd
			mapper->irq_counter = 0;
		} else {       // even
			mapper->irq_load = data;
		}
	} else { // $E000-$FFFF
		if (addr & 1) { // odd
			mapper->irq_enabled = true;
		} else {        // even
			mapper->irq_enabled = false;
			mapper->irq_pending = false;
		}
	}

    return mapper4_map_addr(head, addr);
}

void mapper4_scanline(mapper_t *head) {
    mapper4_t *mapper = (mapper4_t *)head;
	if (mapper->irq_counter > 0)
		--mapper->irq_counter;
	else 
		mapper->irq_counter = mapper->irq_load;

	// NOTE(shaw): this check must happen AFTER decrementing/reloading
	//
	// FUCKING THANK YOU BLARGG!! for pointing this out in your mmc3 test roms
	// see readme.txt: http://slack.net/~ant/old/nes-tests/mmc3_test_2.zip
	//
	// TODO(shaw): blargg also says: "The IRQ flag is not set when the counter is cleared by writing to $C001"
	// so i need to look into that, currently there is no differentiation
	// between counter reaching zero by clocking and by a write to $C001
	if (mapper->irq_counter == 0 && mapper->irq_enabled)
		mapper->irq_pending = true;
}


bool mapper4_irq_pending(mapper_t *head) {
    mapper4_t *mapper = (mapper4_t *)head;
	return mapper->irq_pending;
}

void mapper4_irq_clear(mapper_t *head) {
    mapper4_t *mapper = (mapper4_t *)head;
	mapper->irq_pending = false;
}

/***************************************************************************** 
 * MAPPER 7 
 * 
 * NOTE(shaw): there are a few games that use this mapper that are among the 
 * tricky-to-emulate games: https://www.nesdev.org/wiki/Tricky-to-emulate_games
 * - Battletoads
 *   Infamous among emulator developers for requiring fairly precise CPU and
 *   PPU timing (including the cycle penalty for crossing pages) and a fairly
 *   robust sprite 0 implementation. Because it continuously streams animation
 *   frames into CHR RAM, it leaves rendering disabled for a number of
 *   scanlines into the visible frame to gain extra VRAM upload time and then
 *   enables it. If the timing is off so that the background image appears too
 *   high or too low at this point, a sprite zero hit will fail to trigger,
 *   hanging the game. This usually occurs immediately upon entering the first
 *   stage if the timing is off by enough, and might cause random hangs at
 *   other points otherwise.
 * - Cobra Triangle and Ironsword: Wizards and Warriors II
 *   They rely on the dummy read for the sta $4000,X instruction to acknowledge
 *   pending APU IRQs.
 *
 *   At the time of writing (28 Nov 2022), I don't think I am planning to make
 *   this emulator super cycle accurate, so some of these games like
 *   battletoads might be out of reach. 
 ****************************************************************************/


typedef struct {
    mapper_t head;
    uint8_t reg; // bits 0,1,2,3 selects 32 KB PRG ROM bank for CPU $8000-$FFFF
                 // bit 4 selects 1 KB VRAM page for all 4 nametables
                 //
                 // NOTE(shaw): nesdev says only low 3 bits are used to select
                 // prg bank, however some emulators include bit 4 to allow for
                 // up to 512K roms
} mapper7_t;

static uint32_t
mapper7_map_addr(mapper_t *head, uint16_t addr) {
    mapper7_t *mapper = (mapper7_t *)head;

    if (addr < 0x2000)
        return addr;

    // nametable mirroring
    else if (addr < 0x3F00) {
        uint32_t mapped_addr = addr & 0x0FFF;
        if ((mapper->reg >> 4) & 1)
            return (mapped_addr & 0x03FF) + 0x0400; // one-screen, upper bank
        else
            return mapped_addr & 0x03FF;            // one-screen, lower bank
    } 

    else if (addr < 0x6000)
        return addr;
    else if (addr < 0x8000) /* $6000 - $7FFF */
        return addr - 0x6000;
    else {                  /* $8000 - $FFFF */
        uint8_t prg_bank = (mapper->reg & 0xF) % (head->prg_banks/2);
        return (addr - 0x8000) + (prg_bank * _32KB);
    }
}

static uint32_t 
mapper7_read(mapper_t *head, uint16_t addr) {
    return mapper7_map_addr(head, addr);
}

static uint32_t 
mapper7_write(mapper_t *head, uint16_t addr, uint8_t data) {
    if (addr >= 0x8000) {
        mapper7_t *mapper = (mapper7_t *)head;
        mapper->reg = data;
    }
    return mapper7_map_addr(head, addr);
}





/***************************************************************************** 
 * External API
 ****************************************************************************/
mapper_t *make_mapper(uint16_t mapper_id, uint8_t prg_banks, uint8_t chr_banks, uint8_t mirroring) {
    mapper_t *mapper;

    switch(mapper_id) {
        case 0: {
            mapper0_t *mapper0 = calloc(1, sizeof(mapper0_t));
            mapper = (mapper_t *)mapper0;
            break;
        }
        case 1: {
            mapper1_t *mapper1 = calloc(1, sizeof(mapper1_t));
            mapper = (mapper_t *)mapper1;
            break;
        }
        case 2: {
            mapper2_t *mapper2 = calloc(1, sizeof(mapper2_t));
            mapper = (mapper_t *)mapper2;
            break;
        }
        case 3: {
            mapper3_t *mapper3 = calloc(1, sizeof(mapper3_t));
            mapper = (mapper_t *)mapper3;
            break;
        }
        case 4: {
            mapper4_t *mapper4 = calloc(1, sizeof(mapper4_t));

			uint8_t last = prg_banks*2 - 1;
			uint8_t second_last = last - 1;

			mapper4->prg_bank[0] = 0;
			mapper4->prg_bank[1] = _8KB;
			mapper4->prg_bank[2] = second_last * _8KB;
			mapper4->prg_bank[3] = last * _8KB;
            mapper = (mapper_t *)mapper4;
            break;
		}
        case 7: {
            mapper7_t *mapper7 = calloc(1, sizeof(mapper7_t));
            mapper = (mapper_t *)mapper7;
            break;
        }
        default:
            fprintf(stderr, "Unsupported mapper %d\n", mapper_id);
            exit(1);
            break;
    }

    mapper->prg_banks = prg_banks;
    mapper->chr_banks = chr_banks;
    mapper->mirroring = mirroring;
    mapper->id = mapper_id;

    return mapper;
}


uint32_t mapper_read(mapper_t *mapper, uint16_t addr) {
    switch (mapper->id) {
        case 0: return mapper0_read(mapper, addr);
        case 1: return mapper1_read(mapper, addr);
        case 2: return mapper2_read(mapper, addr);
        case 3: return mapper3_read(mapper, addr);
        case 4: return mapper4_read(mapper, addr);
        case 7: return mapper7_read(mapper, addr);
        default:
            fprintf(stderr, "Unsupported mapper %d\n", mapper->id);
            exit(1);
            break;
    }
}

uint32_t mapper_write(mapper_t *mapper, uint16_t addr, uint8_t data) {
    switch (mapper->id) {
        case 0: return mapper0_write(mapper, addr, data);
        case 1: return mapper1_write(mapper, addr, data);
        case 2: return mapper2_write(mapper, addr, data);
        case 3: return mapper3_write(mapper, addr, data);
        case 4: return mapper4_write(mapper, addr, data);
        case 7: return mapper7_write(mapper, addr, data);
        default:
            fprintf(stderr, "Unsupported mapper %d\n", mapper->id);
            exit(1);
            break;
    }
}

void mapper_scanline(mapper_t *mapper) {
    switch (mapper->id) {
        case 4: mapper4_scanline(mapper);
		default: break;
	}
}

bool mapper_irq_pending(mapper_t *mapper) {
    switch (mapper->id) {
        case 4:  return mapper4_irq_pending(mapper);
		default: return false;
	}
}

void mapper_irq_clear(mapper_t *mapper) {
    switch (mapper->id) {
        case 4:  mapper4_irq_clear(mapper);
		default: break;
	}
}

