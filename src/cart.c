/*
    iNES file format:
    Header (16 bytes)
    Trainer, if present (0 or 512 bytes)
    PRG ROM data (16384 * x bytes)
    CHR ROM data, if present (8192 * y bytes)
    PlayChoice INST-ROM, if present (0 or 8192 bytes)
    PlayChoice PROM, if present (16 bytes Data, 16 bytes CounterOut) (this is often missing, see PC10 ROM-Images for details)

    Header Format:
    0-3: Constant $4E $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
    4: Size of PRG ROM in 16 KB units
    5: Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
    6: Flags 6 - Mapper, mirroring, battery, trainer
    7: Flags 7 - Mapper, VS/Playchoice, NES 2.0
    8: Flags 8 - PRG-RAM size (rarely used extension)
    9: Flags 9 - TV system (rarely used extension)
    10: Flags 10 - TV system, PRG-RAM presence (unofficial, rarely used extension)
    11-15: Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)

    more info: https://www.nesdev.org/wiki/INES
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h> 
#include <stdbool.h> 
#include <string.h>
#include <errno.h>
#include "cart.h"
#include "mappers.h"

#define PRG_RAM_SIZE 8192

typedef struct {
	uint16_t mapper;
	uint8_t prg_rom_banks; // in 16KB units
	uint8_t chr_rom_banks; // in 8KB units
	uint32_t prg_rom_size;
	uint32_t chr_rom_size;
	MirrorMode mirror;
	bool battery_ram;
	bool trainer;
	bool four_screen;
	bool vs_unisystem;
	bool playchoice_10;
	bool nes2_0;
} ines_header_t;

typedef struct {
	ines_header_t header;
    uint8_t *prg_ram;
    uint8_t *prg_rom;
    uint8_t *chr_rom;
    mapper_t *mapper;
} cart_t;

static cart_t cart;

static ines_header_t 
make_ines_header(uint8_t bytes[16]) {
	ines_header_t header = {
		.mapper = (bytes[6] >> 4) | (bytes[7] & 0xF0),
		.prg_rom_banks = bytes[4],
		.chr_rom_banks = bytes[5],
		.prg_rom_size = (uint32_t)bytes[4] << 14,
		.chr_rom_size = (uint32_t)bytes[5] << 13,
		.mirror = bytes[6] & 0x1,
		.battery_ram = bytes[6] & 0x2,
		.trainer = bytes[6] & 0x4,
		.four_screen = bytes[6] & 0x2,
		.vs_unisystem = bytes[7] & 0x1,
		.playchoice_10 = bytes[7] & 0x2,
		.nes2_0 = ((bytes[7] >> 2) & 0x3) == 2,
	};
	return header;
}

void read_rom_file(char *filepath) {
    bool cart_uses_chr_ram = false;
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", filepath, strerror(errno));
        exit(1);
    }

    /* read header */
	uint8_t header_bytes[16];
    if (fread(header_bytes, 1, 16, fp) != 16) {
        fprintf(stderr, "Failed to read %s: %s\n", filepath, strerror(errno));
        exit(1);
    }

    /* verify magic */
    if (0 != strncmp((char *)header_bytes, "NES\x1A", 4)) {
        fprintf(stderr, "Invalid .nes file %s\n", filepath);
        exit(1);
    }

	cart.header = make_ines_header(header_bytes);

    uint32_t prg_rom_size = cart.header.prg_rom_size;
    uint32_t chr_rom_size = cart.header.chr_rom_size;

    if (chr_rom_size == 0) {
        cart_uses_chr_ram = true;
        chr_rom_size = PRG_RAM_SIZE; 
    }

    cart.prg_rom = malloc(prg_rom_size);
    cart.chr_rom = malloc(chr_rom_size);
    /* TODO(shaw): this will probably depend on mapper */
    cart.prg_ram = malloc(PRG_RAM_SIZE); 

    cart.mapper = make_mapper(
		cart.header.mapper,
		cart.header.prg_rom_banks,
		cart.header.chr_rom_banks,
		cart.header.mirror);

    printf("%u * 16kB ROM, %u * 8kB VROM, mapper %u, %s mirroring\n", 
			cart.header.prg_rom_banks,
			cart.header.chr_rom_banks,
			cart.header.mapper,
			cart.header.mirror ? "vertical" : "horizontal");

    /* ignore trainer for now */
    /* TODO(shaw): implement trainer ?? */
    if (cart.header.trainer) {
        printf("Warning: cart contains trainer but this emulator does not support them.\n");
        if(fseek(fp, 512, SEEK_CUR) < 0) {
            perror("fseek");
            exit(1);
        }
    }

    /* read prg rom */
    if (fread(cart.prg_rom, 1, prg_rom_size, fp) != prg_rom_size) {
        fprintf(stderr, "Failed to read PRG ROM from %s: %s\n", filepath, strerror(errno));
        exit(1);
    }
 
    /* read chr rom */
    if (!cart_uses_chr_ram && fread(cart.chr_rom, 1, chr_rom_size, fp) != chr_rom_size) {
        fprintf(stderr, "Failed to read CHR ROM from %s: %s\n", filepath, strerror(errno));
        exit(1);
    }

    if (fp) fclose(fp);
}

void delete_cart() {
    free(cart.prg_rom);
    free(cart.chr_rom);
    free(cart.prg_ram);
    memset(&cart, 0, sizeof(cart_t));
}

uint8_t cart_cpu_read(uint16_t addr) {
    uint32_t mapped_addr = mapper_read(cart.mapper, addr);

	if (addr < 0x4020) {
		assert(0 && "cpu should only access cartridge from 0x4020-0xFFFF");
		return 0;
	} else if (addr < 0x6000) {
        // NOTE(shaw): currently ignoring 0x4020 - 0x5FFF, but some mappers use this address space
        // https://www.nesdev.org/wiki/Category:Mappers_using_$4020-$5FFF
        return 0;
    } else if (addr < 0x8000) { /* $6000 - $7FFF */
		assert(mapped_addr < PRG_RAM_SIZE);
        return cart.prg_ram[mapped_addr];
	} else {
        /*printf("addr=0x%4X mapped_addr=0x%4X\n", addr, mapped_addr);*/
		assert(mapped_addr < cart.header.prg_rom_size);
        return cart.prg_rom[mapped_addr];
    }
}

void cart_cpu_write(uint16_t addr, uint8_t data) {
    uint32_t mapped_addr = mapper_write(cart.mapper, addr, data);

    if (addr < 0x4020) 
        assert(0 && "cpu should only access cartridge from 0x4020-0xFFFF");
    else if (addr < 0x6000) {
        // NOTE(shaw): currently ignoring 0x4020 - 0x5FFF, but some mappers use this address space
        // https://www.nesdev.org/wiki/Category:Mappers_using_$4020-$5FFF
    } else if (addr < 0x8000) { /* $6000 - $7FFF */
		assert(mapped_addr < PRG_RAM_SIZE);
        cart.prg_ram[mapped_addr] = data;
	}

    /*if (addr < 0x2000) */
        /*cart.chr_rom[addr] = data;*/
    /*else if (addr < 0x6000)*/
        /*assert(0 && "this part of cart memory not implemented");*/
    /*else if (addr < 0x8000)*/
        /*cart.prg_ram[addr - 0x6000] = data;*/
}


uint8_t cart_ppu_read(uint16_t addr, uint8_t vram[2048]) {
    uint32_t mapped_addr = mapper_read(cart.mapper, addr);

    return addr < 0x2000
        ? cart.chr_rom[mapped_addr]
        : vram[mapped_addr];
}

void cart_ppu_write(uint16_t addr, uint8_t data, uint8_t vram[2048]) {
    uint32_t mapped_addr = mapper_write(cart.mapper, addr, data);

    if (addr < 0x2000)
        cart.chr_rom[mapped_addr] = data;
    else 
        vram[mapped_addr] = data;
}



