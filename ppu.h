#ifndef _PPU_H
#define _PPU_H

#include <stdint.h>

uint8_t ppu_read(uint16_t addr);
void ppu_write(uint16_t addr, uint8_t data);
void ppu_tick(void);

#endif
