#include <stdbool.h>
#include "ppu.h"
#include "io.h"

#include <stdlib.h> /* TEMP just for rand */

/*
 * PPU Memory Map
 *  $0000-0FFF     Pattern table 0
 *  $1000-1FFF     Pattern table 1
 *  $2000-23FF     Nametable 0
 *  $2400-27FF     Nametable 1
 *  $2800-2BFF     Nametable 2
 *  $2C00-2FFF     Nametable 3
 *  $3000-3EFF     Mirrors of $2000-$2EFF
 *  $3F00-3F1F     Palette RAM indexes
 *  $3F20-3FFF     Mirrors of $3F00-$3F1F
 */

static uint32_t *screen_pixels;

typedef struct {
    uint8_t vram[2048];
    uint8_t palette_ram[32];
    uint8_t OAM[256];
    uint8_t registers[9];
    uint8_t OAMDMA;
    uint8_t data_buffer;

    uint32_t colors[64];
    bool even_odd_toggle;
    bool offset_toggle; /* used for writing low or high byte into address */
    bool frame_completed;
    bool nmi;
    uint16_t cycle, scanline;
    uint16_t address;
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

static ppu_t ppu = { 
    .colors = { 0xff525252, 0xff511a01, 0xff650f0f, 0xff630623, 0xff4b0336, 0xff260440, 0xff04093f, 0xff001332, 0xff00201f, 0xff002a0b, 0xff002f00, 0xff0a2e00, 0xff2d2600, 0xff000000, 0xff000000, 0xff000000, 0xffa0a0a0, 0xff9d4a1e, 0xffbc3738, 0xffb82858, 0xff942175, 0xff5c2384, 0xff242e82, 0xff003f6f, 0xff005251, 0xff006331, 0xff056b1a, 0xff2e690e, 0xff685c10, 0xff000000, 0xff000000, 0xff000000, 0xfffffffe, 0xfffc9e69, 0xffff8789, 0xffff76ae, 0xfff16dce, 0xffb270e0, 0xff707cde, 0xff3e91c8, 0xff25a7a6, 0xff28ba81, 0xff46c463, 0xff7dc154, 0xffc0b356, 0xff3c3c3c, 0xff000000, 0xff000000, 0xfffffffe, 0xfffdd6be, 0xffffcccc, 0xffffc4dd, 0xfff9c0ea, 0xffdfc1f2, 0xffc2c7f1, 0xffaad0e8, 0xff9ddad9, 0xff9ee2c9, 0xffaee6bc, 0xffc7e5b4, 0xffe4dfb5, 0xffa9a9a9, 0xff000000, 0xff000000 } };

#define CTRL_NMI              ((ppu.registers[PPUCTRL] >> 7) & 1)
#define CTRL_MASTER           ((ppu.registers[PPUCTRL] >> 6) & 1)
#define CTRL_EIGHT_BY_SIXTEEN ((ppu.registers[PPUCTRL] >> 5) & 1)
#define CTRL_BG_TABLE         ((ppu.registers[PPUCTRL] >> 4) & 1)
#define CTRL_SPRITE_TABLE     ((ppu.registers[PPUCTRL] >> 3) & 1)
#define CTRL_INC_VERTICAL     ((ppu.registers[PPUCTRL] >> 2) & 1)
#define CTRL_BASE_NAMETABLE   (ppu.registers[PPUCTRL] & 0x3)

static uint8_t 
ppu_bus_read(uint16_t addr) {
    uint8_t data = 0;
    if (addr < 0x2000) {
        /*cart_read()*/
    } else if (addr < 0x3F00) {
        /*vram read*/
    } else {
        data = ppu.palette_ram[(addr - 0x3F00) & 0xFF];
    }

    return data;
}

static void 
ppu_bus_write(uint16_t addr, uint8_t data) {
    if (addr < 0x2000) {
        /*cart_write()*/
    } else if (addr < 0x3F00) {
        /*vram write */
    } else {
        ppu.palette_ram[(addr - 0x3F00) & 0xFF] = data;
    }
}

void ppu_init(uint32_t *pixels) {
    screen_pixels = pixels;
    /* palette generate from bisquit.iki.fi/utils/nespalette/php 
     * this format is ARGB */
}


/* used just for debug drawing palettes */
void update_palettes(sprite_t palettes[8]) {
    uint32_t bg_color = ppu.colors[ppu.palette_ram[0]];
    for (int i=0; i<8; ++i) {
        uint32_t *pixels = palettes[i].pixels;
        for (int p=0; p<3; ++p) {
            int index = ppu.palette_ram[i*4+1];
            pixels[p] = ppu.colors[index];
        }
        pixels[3] = bg_color;
    }
}

/* used just for debug drawing pattern_tables */
void update_pattern_tables(sprite_t pattern_tables[8]) {
    (void)pattern_tables;
}


/* reads from cpu, addr is 0-7 */
uint8_t ppu_read(uint16_t addr) {
    uint8_t data = 0;
    switch (addr) {
        case PPUCTRL: 
            break;
        case PPUMASK: 
            break;
        case PPUSTATUS: 
        {
            data = (ppu.registers[PPUSTATUS] & 0xE0) | (ppu.data_buffer & 0x1F);
            ppu.registers[PPUSTATUS] &= ~0x80; /* clear vblank */
            ppu.offset_toggle = 0;
            break;
        }
        case OAMADDR: 
            break;
        case OAMDATA: 
            break;
        case PPUSCROLL: 
            break;
        case PPUADDR:
            break;
        case PPUDATA:
        {
            data = ppu.data_buffer;
            ppu.data_buffer = ppu_bus_read(ppu.address);
            if (ppu.address > 0x3EFF) data = ppu.data_buffer;
            ++ppu.address;
            break;
        }
        default:
            break;
    }
    return data;
}


/* writes from cpu, addr is 0-7 */
void ppu_write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case PPUCTRL: 
            ppu.registers[PPUCTRL] = data;
            break;
        case PPUMASK: 
            ppu.registers[PPUMASK] = data;
            break;
        case PPUSTATUS:
            break;
        case OAMADDR:
            break;
        case OAMDATA:
            break;
        case PPUSCROLL:
        {
            /*if (ppu.offset_toggle)*/
                /*[>y scroll<]*/
            /*else */
                /*[>x scroll<]*/
            break;
        }
        case PPUADDR:
        {
            if (ppu.offset_toggle)
                ppu.address = (ppu.address & 0xFF00) | data;
            else
                ppu.address = (ppu.address & 0x00FF) | (data << 8);
            ppu.offset_toggle = !ppu.offset_toggle;
            break;
        } 
        case PPUDATA:
            ppu_bus_write(ppu.address++, data);
            break;
        default:
            break;
    }
}

void ppu_tick(void) {
    
    if (ppu.scanline < 240) { /* visible scanlines */
        if (ppu.cycle < 256) {
            uint8_t val = rand() % 256;
            screen_pixels[ppu.scanline * NES_WIDTH + ppu.cycle] = (val << 16) | (val << 8) | val;
        }
    } else if (ppu.scanline == 241 && ppu.cycle == 1) {
        ppu.registers[PPUSTATUS] |= 0x80;      /* set vblank */
        if (CTRL_NMI)
            ppu.nmi = true;
    } else if (ppu.scanline == 261) {
        if (ppu.cycle == 1)
            ppu.registers[PPUSTATUS] &= ~0x80; /* clear vblank */
        else if (ppu.cycle == 239 && ppu.even_odd_toggle) { /* TODO: && bg_rendering_enabled */
            /* skip cycle 0 idle on odd ticks when bg enabled */
            ppu.cycle = 0;
            ppu.scanline = 0;
            ppu.frame_completed = true;
            ppu.even_odd_toggle = !ppu.even_odd_toggle;
            return;
        }
    }

    if (++ppu.cycle > 340) {
        ppu.cycle = 0;
        if (++ppu.scanline > 261) {
            ppu.scanline = 0;
            ppu.frame_completed = true;
            ppu.even_odd_toggle = !ppu.even_odd_toggle;
        }
    }
}

bool ppu_frame_completed(void) {
    return ppu.frame_completed;
}

void ppu_clear_frame_completed(void) {
    ppu.frame_completed = false;
}

bool ppu_nmi(void) {
    return ppu.nmi;
}
