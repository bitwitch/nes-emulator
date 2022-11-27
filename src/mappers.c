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
 * MAPPER 2 
 ****************************************************************************/
typedef struct {
    mapper_t head;
    uint8_t bank;
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
        return (addr - 0x8000) + (mapper->bank * _16KB);
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
        mapper->bank = data;
    }
    return mapper2_map_addr(head, addr);
}



/***************************************************************************** 
 * MAPPER 3 
 ****************************************************************************/
typedef struct {
    mapper_t head;
    uint8_t bank; // mapper 3 has up to 2048KiB of bank switchable CHR rom
} mapper3_t;

static uint32_t
mapper3_map_addr(mapper_t *head, uint16_t addr) {
    mapper3_t *mapper = (mapper3_t *)head;

    if (addr < 0x2000) {
        // ppu bank switchable chr rom
        // NOTE(shaw): banks not used by the cartridge are mirrored over banks
        // that are used
        return addr + ((mapper->bank % head->chr_banks) * _8KB);
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
        mapper->bank = data;
    }
    return mapper3_map_addr(head, addr);
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
        case 2: return mapper2_read(mapper, addr);
        case 3: return mapper3_read(mapper, addr);
        default:
            fprintf(stderr, "Unsupported mapper %d\n", mapper->id);
            exit(1);
            break;
    }
}

uint32_t mapper_write(mapper_t *mapper, uint16_t addr, uint8_t data) {
    switch (mapper->id) {
        case 0: return mapper0_write(mapper, addr, data);
        case 2: return mapper2_write(mapper, addr, data);
        case 3: return mapper3_write(mapper, addr, data);
        default:
            fprintf(stderr, "Unsupported mapper %d\n", mapper->id);
            exit(1);
            break;
    }
}


