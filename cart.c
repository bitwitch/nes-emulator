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

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <errno.h>
#include "cart.h"

cart_t ines_read(char *filepath) {
    cart_t cart = {0};
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

    uint32_t prg_rom_size = CART_PRG_ROM_SIZE(cart.header);
    uint32_t chr_rom_size = CART_CHR_ROM_SIZE(cart.header);
    cart.prg_rom = malloc(prg_rom_size * sizeof(uint8_t));
    cart.chr_rom = malloc(chr_rom_size * sizeof(uint8_t));

    /* ignore trainer for now */
    /* TODO(shaw): implement trainer ?? */
    if (CART_TRAINER(cart.header)) {
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
    return cart;
}

void delete_cart(cart_t *cart) {
    free(cart->prg_rom);
    free(cart->chr_rom);
}


