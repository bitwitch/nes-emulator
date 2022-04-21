#ifndef _CART_H_
#define _CART_H_

#include <stdint.h>

void read_rom_file(char *filepath);
void delete_cart();

uint8_t cart_read(uint16_t addr);
void cart_write(uint16_t addr, uint8_t data);

#endif
