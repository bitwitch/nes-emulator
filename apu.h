#ifndef __APU_H__
#define __APU_H__

#include <stdint.h>

void apu_init(void);
uint8_t apu_read(uint16_t addr);
void apu_write(uint16_t addr, uint8_t data);

#endif
