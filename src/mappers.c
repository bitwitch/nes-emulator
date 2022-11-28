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

#include "mappers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


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
        uint16_t mapped_addr = (addr & 0x2FFF) - 0x2000;
        if (head->mirroring == MAPPER_MIRROR_VERT)
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
        uint32_t mapped_addr = (addr & 0x2FFF) - 0x2000;
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
            break;
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
                break;
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
                break;
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
        uint32_t mapped_addr = (addr & 0x2FFF) - 0x2000;
        if (head->mirroring == MAPPER_MIRROR_VERT)
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
        uint32_t mapped_addr = (addr & 0x2FFF) - 0x2000;
        if (head->mirroring == MAPPER_MIRROR_VERT)
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
        uint32_t mapped_addr = (addr & 0x2FFF) - 0x2000;
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
        case 0:
        {
            mapper0_t *mapper0 = calloc(1, sizeof(mapper0_t));
            mapper = (mapper_t *)mapper0;
            break;
        }
        case 1:
        {
            mapper1_t *mapper1 = calloc(1, sizeof(mapper1_t));
            mapper = (mapper_t *)mapper1;
            break;
        }
        case 2:
        {
            mapper2_t *mapper2 = calloc(1, sizeof(mapper2_t));
            mapper = (mapper_t *)mapper2;
            break;
        }
        case 3:
        {
            mapper3_t *mapper3 = calloc(1, sizeof(mapper3_t));
            mapper = (mapper_t *)mapper3;
            break;
        }
        case 7:
        {
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
        case 7: return mapper7_write(mapper, addr, data);
        default:
            fprintf(stderr, "Unsupported mapper %d\n", mapper->id);
            exit(1);
            break;
    }
}


