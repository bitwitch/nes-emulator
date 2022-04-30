#ifndef __BUS_H
#define __BUS_H

#include "cpu_6502.h"
#include <stdint.h>


void init_memory(void);
void load_memory(uint16_t addr, uint8_t *data, uint32_t size);
uint8_t bus_read(uint16_t addr);
void bus_write(uint16_t addr, uint8_t data);
void system_reset(cpu_t *cpu);

#endif
