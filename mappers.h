#ifndef _MAPPER_H_
#define _MAPPER_H_

#include <stdint.h>
#include <stdbool.h>

#define MAPPER_MIRROR_HORIZ   0
#define MAPPER_MIRROR_VERT    1

typedef struct {
    uint8_t prg_banks;    /* number of 16KB prg rom banks */
    uint8_t chr_banks;    /* number of  8KB chr rom banks */
    uint8_t mirroring;    /* 0 == horizontal 1 == vertical */
    uint16_t id;          /* mapper number */
} mapper_t;

mapper_t *make_mapper(uint16_t mapper_id, uint8_t prg_banks, uint8_t chr_banks, uint8_t mirroring);
uint16_t mapper_read(mapper_t *mapper, uint16_t addr);
uint16_t mapper_write(mapper_t *mapper, uint16_t addr, uint8_t data);

#endif 
