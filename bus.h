#ifndef __BUS_H
#define __BUS_H

#include <stdint.h>

void init_memory(void);
void load_memory(uint16_t addr, uint8_t *data, uint16_t size);

#endif
