#include <stdbool.h>
#include "ppu.h"

typedef struct {
    uint8_t palette_ram[0x20];
    uint8_t OAM[0x100];
    uint8_t registers[9];
    uint8_t OAMDMA;


    bool even;

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

uint8_t ppu_read(uint16_t addr) {
    /* TODO(shaw): handle OAMDMA, 0x4014 */
    return ppu.registers[(addr - 0x2000) & 0xF];
}

void ppu_write(uint16_t addr, uint8_t data) {
    /* TODO(shaw): handle OAMDMA, 0x4014 */
    ppu.registers[(addr - 0x2000) & 0xF] = data;
}

void ppu_tick(void) {

}
