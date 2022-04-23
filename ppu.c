#include <stdbool.h>
#include "ppu.h"
#include "io.h"

#include <stdlib.h> /* TEMP just for rand */

static uint32_t *screen_pixels;

typedef struct {
    uint8_t vram[2048];
    uint8_t palette_ram[32];
    uint8_t OAM[256];
    uint8_t registers[9];
    uint8_t OAMDMA;

    bool even_odd_toggle; 
    uint16_t scanline;
    uint16_t cycle;
} ppu_t;

enum {
   PPUCTRL = 0,         /* $2000 */
   PPUMASK,             /* $2001 */
   PPUSTATUS,           /* $2002 */
   OAMADDR,             /* $2003 */
   OAMDATA,             /* $2004 */
   PPUSCROLL,           /* $2005 */
   PPUADDR,             /* $2006 */
   PPUDATA              /* $2007 */
};

static ppu_t ppu;

void ppu_init(uint32_t *pixels) {
    screen_pixels = pixels;
}

uint8_t ppu_read(uint16_t addr) {
    /* TODO(shaw): handle OAMDMA, 0x4014 */
    return ppu.registers[(addr - 0x2000) & 0xF];
}

void ppu_write(uint16_t addr, uint8_t data) {
    /* TODO(shaw): handle OAMDMA, 0x4014 */
    ppu.registers[(addr - 0x2000) & 0xF] = data;
}

void ppu_tick(void) {
    if (ppu.scanline < 240) {
        if (ppu.cycle < 256) {
            uint8_t val = rand() % 256;
            screen_pixels[ppu.scanline * WIDTH + ppu.cycle] = (val << 16) | (val << 8) | val;
        }
    }

    if (++ppu.cycle > 340) {
        ppu.cycle = 0;
        if (++ppu.scanline > 261) ppu.scanline = 0;
    }
}
