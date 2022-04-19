#ifndef _CART_H_
#define _CART_H_

#include <stdint.h>

typedef struct {
    uint8_t header[16];
    uint8_t *prg_rom;
    uint8_t *chr_rom;
} cart_t;

#define CART_PRG_ROM_BANKS(header) (header[4])
#define CART_PRG_ROM_SIZE(header)  ((uint32_t)header[4]<<14)
#define CART_CHR_ROM_BANKS(header) (header[5])
#define CART_CHR_ROM_SIZE(header)  ((uint32_t)header[5]<<13)
#define CART_MIRROR(header)        (header[6]&0x01)
#define CART_BATTERY_RAM(header)   (header[6]&0x02)
#define CART_TRAINER(header)       (header[6]&0x04)
#define CART_FOUR_SCREEN(header)   (header[6]&0x08)
#define CART_MAPPER(header)        ((header[6]>>4)|(header[7]&0xF0))
#define CART_VSUNISYSTEM(header)   (header[7]&0x01)


cart_t ines_read(char *filepath);
void delete_cart(cart_t *cart);


#endif
