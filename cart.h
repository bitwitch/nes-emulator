#ifndef _CART_H_
#define _CART_H_

#include <stdint.h>

typedef struct {
    uint8_t header[16];
    uint8_t *prg_ram;
    uint8_t *prg_rom;
    uint8_t *chr_rom;
} cart_t;

#define PRG_ROM_BANKS(header) (header[4])
#define PRG_ROM_SIZE(header)  ((uint32_t)header[4]<<14)
#define CHR_ROM_BANKS(header) (header[5])
#define CHR_ROM_SIZE(header)  ((uint32_t)header[5]<<13)
#define CONTROL_BYTE(header)  (header[6])
#define MIRROR(header)        (header[6]&0x01)
#define BATTERY_RAM(header)   (header[6]&0x02)
#define TRAINER(header)       (header[6]&0x04)
#define FOUR_SCREEN(header)   (header[6]&0x08)
#define MAPPER(header)        ((header[6]>>4)|(header[7]&0xF0))
#define VSUNISYSTEM(header)   (header[7]&0x01)

void read_rom_file(char *filepath);
void delete_cart();

uint8_t cart_read(uint16_t addr);
void cart_write(uint16_t addr, uint8_t data);

#endif
