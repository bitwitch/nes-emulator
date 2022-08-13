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
#include <string.h>
#include <errno.h>
#include "cart.h"
#include "mappers.h"

/* TODO(shaw): store size of rom arrays and do some assert bounds checking on array accesses */
typedef struct {
    uint8_t header[16];
    uint8_t *prg_ram;
    uint8_t *prg_rom;
    uint8_t *chr_rom;
    mapper_t *mapper;
} cart_t;

#define PRG_ROM_BANKS(header) (header[4])
#define PRG_ROM_SIZE(header)  ((uint32_t)header[4]<<14)
#define CHR_ROM_BANKS(header) (header[5])
#define CHR_ROM_SIZE(header)  ((uint32_t)header[5]<<13)
#define CONTROL_BYTE(header)  (header[6])
#define MIRROR(header)        (header[6]&0x01) /* 0 == horizontal, 1 == vertical */
#define BATTERY_RAM(header)   (header[6]&0x02)
#define TRAINER(header)       (header[6]&0x04)
#define FOUR_SCREEN(header)   (header[6]&0x08)
#define MAPPER(header)        ((header[6]>>4)|(header[7]&0xF0))
#define VSUNISYSTEM(header)   (header[7]&0x01)

static cart_t cart;

void read_rom_file(char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", filepath, strerror(errno));
        exit(1);
    }

    /* read header */
    if (fread(cart.header, 1, 16, fp) != 16) {
        fprintf(stderr, "Failed to read %s: %s\n", filepath, strerror(errno));
        exit(1);
    }

    /* verify magic */
    if (0 != strncmp((char *)cart.header, "NES\x1A", 4)) {
        fprintf(stderr, "Invalid .nes file %s\n", filepath);
        exit(1);
    }

    uint32_t prg_rom_size = PRG_ROM_SIZE(cart.header);
    uint32_t chr_rom_size = CHR_ROM_SIZE(cart.header);
    cart.prg_rom = malloc(prg_rom_size);
    cart.chr_rom = malloc(chr_rom_size);
    /* TODO(shaw): this will probably depend on mapper */
    cart.prg_ram = malloc(8192); 

    cart.mapper = make_mapper(
        MAPPER(cart.header), 
        PRG_ROM_BANKS(cart.header), 
        CHR_ROM_BANKS(cart.header),
        MIRROR(cart.header));

    printf("%u * 16kB ROM, %u * 8kB VROM, mapper %u, %s mirroring\n", PRG_ROM_BANKS(cart.header), CHR_ROM_BANKS(cart.header), MAPPER(cart.header), MIRROR(cart.header) ? "vertical" : "horizontal");

    /* ignore trainer for now */
    /* TODO(shaw): implement trainer ?? */
    if (TRAINER(cart.header)) {
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
    if (fread(cart.chr_rom, 1, chr_rom_size, fp) != chr_rom_size) {
        fprintf(stderr, "Failed to read CHR ROM from %s: %s\n", filepath, strerror(errno));
        exit(1);
    }

    /* TODO(shaw): implement the rest of the ines file reading, right now I am
     * just reading the prg rom and chr rom, since all i really need want to do
     * is load a 6502 test file to test my 6502 emulator */
       
    if (fp) fclose(fp);
}

void delete_cart() {
    free(cart.prg_rom);
    free(cart.chr_rom);
    free(cart.prg_ram);
    memset(&cart, 0, sizeof(cart_t));
}

uint8_t cart_cpu_read(uint16_t addr) {
    uint16_t mapped_addr = mapper_read(cart.mapper, addr);

    if (addr < 0x4020) 
        assert(0 && "cpu should only access cartridge from 0x4020-0xFFFF");
    else if (addr < 0x6000) 
        assert(0 && "this part of cart memory not implemented");
    else if (addr < 0x8000) /* $6000 - $7FFF */
        return cart.prg_ram[mapped_addr];
    else {
        /*printf("addr=0x%4X mapped_addr=0x%4X\n", addr, mapped_addr);*/
        return cart.prg_rom[mapped_addr];
    }

}

void cart_cpu_write(uint16_t addr, uint8_t data) {
    uint16_t mapped_addr = mapper_write(cart.mapper, addr, data);

    if (addr < 0x4020) 
        assert(0 && "cpu should only access cartridge from 0x4020-0xFFFF");
    else if (addr < 0x6000)
        assert(0 && "this part of cart memory not implemented");
    else if (addr < 0x8000) /* $6000 - $7FFF */
        cart.prg_ram[mapped_addr] = data;


    /*if (addr < 0x2000) */
        /*cart.chr_rom[addr] = data;*/
    /*else if (addr < 0x6000)*/
        /*assert(0 && "this part of cart memory not implemented");*/
    /*else if (addr < 0x8000)*/
        /*cart.prg_ram[addr - 0x6000] = data;*/

}


uint8_t cart_ppu_read(uint16_t addr, uint8_t vram[2048]) {
    uint16_t mapped_addr = mapper_read(cart.mapper, addr);
    return addr < 0x2000
        ? cart.chr_rom[mapped_addr]
        : vram[mapped_addr];

}

void cart_ppu_write(uint16_t addr, uint8_t data, uint8_t vram[2048]) {
    uint16_t mapped_addr = mapper_write(cart.mapper, addr, data);
    if (addr < 0x2000)
        cart.chr_rom[mapped_addr] = data;
    else 
        vram[mapped_addr] = data;
}



