#ifndef _MAPPER_H_
#define _MAPPER_H_

#include <stdint.h>
#include <stdbool.h>

#define _4KB                   4096
#define _8KB                   8192
#define _16KB                  16384
#define _32KB                  32768

typedef enum {
	MIRROR_HORIZONAL,
	MIRROR_VERTICAL,
} MirrorMode;

typedef struct {
    uint8_t prg_banks;    /* number of 16KB prg rom banks */
    uint8_t chr_banks;    /* number of  8KB chr rom banks */
    MirrorMode mirroring; /* 0 == horizontal 1 == vertical */
    uint16_t id;          /* mapper number */
} mapper_t;

mapper_t *make_mapper(uint16_t mapper_id, uint8_t prg_banks, uint8_t chr_banks, MirrorMode mirroring);
uint32_t mapper_read(mapper_t *mapper, uint16_t addr);
uint32_t mapper_write(mapper_t *mapper, uint16_t addr, uint8_t data);

#endif 
