#include <stdbool.h>
#include <stdlib.h> /* for rand */
#include "ppu.h"
#include "io.h"
#include "cart.h"


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



/*
 *  Loopy Registers:
 *      11 00000 00000
 *  yyy NN YYYYY XXXXX
 *  ||| || ||||| +++++-- coarse X scroll
 *  ||| || +++++-------- coarse Y scroll
 *  ||| ++-------------- nametable select
 *  +++----------------- fine Y scroll
*/
typedef union {
    uint16_t reg;
    struct {
        uint16_t coarse_x  : 5;
        uint16_t coarse_y  : 5;
        uint16_t nt_select : 2;
        uint16_t fine_y    : 3;
    } bits;
} loopy_t;

typedef struct {
    uint8_t vram[2048];
    uint8_t palette_ram[32];
    uint8_t OAM[256];
    uint8_t registers[9];
    uint8_t OAMDMA;
    uint8_t data_buffer;

    uint32_t colors[64];
    uint32_t *screen_pixels;
    bool even_odd_toggle;
    bool frame_completed;
    bool nmi_occured;
    uint16_t cycle, scanline;

    /* temp */
    uint16_t address;
    /* temp */

    loopy_t vram_addr, vram_temp;
    uint8_t fine_x;
    bool offset_toggle; /* used for writing low or high byte into address */

    uint8_t nt_byte;
    uint16_t bg_shifter_pat_lo;
    uint16_t bg_shifter_pat_hi;
    uint8_t bg_shifter_attr_lo;
    uint8_t bg_shifter_attr_hi;
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
    /* palette generate from bisquit.iki.fi/utils/nespalette/php 
     * this format is ARGB */
    .colors = { 0xff525252, 0xff511a01, 0xff650f0f, 0xff630623, 0xff4b0336, 0xff260440, 0xff04093f, 0xff001332, 0xff00201f, 0xff002a0b, 0xff002f00, 0xff0a2e00, 0xff2d2600, 0xff000000, 0xff000000, 0xff000000, 0xffa0a0a0, 0xff9d4a1e, 0xffbc3738, 0xffb82858, 0xff942175, 0xff5c2384, 0xff242e82, 0xff003f6f, 0xff005251, 0xff006331, 0xff056b1a, 0xff2e690e, 0xff685c10, 0xff000000, 0xff000000, 0xff000000, 0xfffffffe, 0xfffc9e69, 0xffff8789, 0xffff76ae, 0xfff16dce, 0xffb270e0, 0xff707cde, 0xff3e91c8, 0xff25a7a6, 0xff28ba81, 0xff46c463, 0xff7dc154, 0xffc0b356, 0xff3c3c3c, 0xff000000, 0xff000000, 0xfffffffe, 0xfffdd6be, 0xffffcccc, 0xffffc4dd, 0xfff9c0ea, 0xffdfc1f2, 0xffc2c7f1, 0xffaad0e8, 0xff9ddad9, 0xff9ee2c9, 0xffaee6bc, 0xffc7e5b4, 0xffe4dfb5, 0xffa9a9a9, 0xff000000, 0xff000000 } };

#define CTRL_NMI              ((ppu.registers[PPUCTRL] >> 7) & 1)
#define CTRL_MASTER           ((ppu.registers[PPUCTRL] >> 6) & 1)
#define CTRL_EIGHT_BY_SIXTEEN ((ppu.registers[PPUCTRL] >> 5) & 1)
#define CTRL_BG_TABLE         ((ppu.registers[PPUCTRL] >> 4) & 1)
#define CTRL_SPRITE_TABLE     ((ppu.registers[PPUCTRL] >> 3) & 1)
#define CTRL_INC_VERTICAL     ((ppu.registers[PPUCTRL] >> 2) & 1)
#define CTRL_BASE_NAMETABLE   (ppu.registers[PPUCTRL] & 0x3)

#define MASK_EMP_B            ((ppu.registers[PPUMASK] >> 7) & 1)
#define MASK_EMP_G            ((ppu.registers[PPUMASK] >> 6) & 1)
#define MASK_EMP_R            ((ppu.registers[PPUMASK] >> 5) & 1)
#define MASK_SHOW_SPR         ((ppu.registers[PPUMASK] >> 4) & 1)
#define MASK_SHOW_BG          ((ppu.registers[PPUMASK] >> 3) & 1)
#define MASK_SHOW_SPR_LEFT    ((ppu.registers[PPUMASK] >> 2) & 1)
#define MASK_SHOW_BG_LEFT     ((ppu.registers[PPUMASK] >> 1) & 1)
#define MASK_GREY             ((ppu.registers[PPUMASK] >> 0) & 1)

static uint8_t 
ppu_bus_read(uint16_t addr) {
    uint8_t data = 0;
    if (addr < 0x2000) {
        data = cart_read(addr);
    } else if (addr < 0x3F00) {
        /* TODO(shaw): do this with mappers 
         * right now, just hardcoding vertical mirroring */
        addr = (addr - 0x2000) & 0x07FF;
        return ppu.vram[addr];
    } else {
        addr = (addr - 0x3F00) & 0x1F;
        if (addr % 4 == 0) addr = 0;
        data = ppu.palette_ram[addr];
    }

    return data;
}

static void 
ppu_bus_write(uint16_t addr, uint8_t data) {
    if (addr < 0x2000) {
        /*cart_write()*/
    } else if (addr < 0x3F00) {
        /* TODO(shaw): do this with mappers 
         * right now, just hardcoding vertical mirroring */
        addr = (addr - 0x2000) & 0x07FF;
        ppu.vram[addr] = data;
    } else {
        addr = (addr - 0x3F00) & 0x1F;
        if (addr % 4 == 0) addr = 0;
        ppu.palette_ram[addr] = data;
    }
}

static uint32_t
get_color_from_palette(uint8_t palette_num, uint8_t palette_index) {
    uint16_t addr = 0x3F01 + palette_num*4 + palette_index;
    uint8_t index = ppu_bus_read(addr);
    return ppu.colors[index];
}

void ppu_init(uint32_t *pixels) {
    ppu.screen_pixels = pixels;
    /* initialize with some random colors to visualize palette on load */
    for (int i=0; i<32; ++i) {
        int r = rand();
        ppu.palette_ram[i] = (uint8_t)(r >> 6);
        /*ppu.palette_ram[i] = (uint8_t)pixels[i];*/
    }
}


/* used just for debug drawing palettes */
void update_palettes(sprite_t palettes[8]) {
    uint32_t bg_color = ppu.colors[ppu_bus_read(0x3F00)];
    for (int i=0; i<8; ++i) {
        uint32_t *pixels = palettes[i].pixels;
        for (int p=0; p<3; ++p) {
            pixels[p] = get_color_from_palette(i, p);
        }
        pixels[3] = bg_color;
    }
}

/*
row 1
0x0000 - 0x000F
0x0010 - 0x001F
0x0020 - 0x002F
0x0030 - 0x003F
0x0040 - 0x004F
0x0050 - 0x005F
0x0060 - 0x006F
0x0070 - 0x007F
0x0080 - 0x008F
0x0090 - 0x009F
0x00a0 - 0x00aF
0x00b0 - 0x00bF
0x00c0 - 0x00cF
0x00d0 - 0x00dF
0x00e0 - 0x00eF
0x00f0 - 0x00fF

row 2
0x0100 - 0x010F
0x0110 - 0x011F
0x0120 - 0x012F

w = 0x100
row * w + col
*/

/* used just for debug drawing pattern_tables */
void update_pattern_tables(int selected_palette, sprite_t pattern_tables[2]) {
    int pitch = pattern_tables[0].surface->pitch;
    /*for each pattern table*/
    for (int table=0; table<2; ++table)
    /*for each tile in the pattern table*/
    for (uint16_t tile_row=0; tile_row<16; ++tile_row)
    for (uint16_t tile_col=0; tile_col<16; ++tile_col) {
        uint16_t tile_start = table*0x1000 + tile_row*0x100 + tile_col*0x10;
        uint16_t pixel_start = (tile_row*pitch+tile_col)*8;
        /*for each row in the tile*/
        for (int tile_y=0; tile_y<8; ++tile_y) {
            uint8_t plane0 = ppu_bus_read(tile_start+tile_y);
            uint8_t plane1 = ppu_bus_read(tile_start+tile_y+8);
            /*for each col in the tile*/
            for (int tile_x=0; tile_x<8; ++tile_x) {
                int i = pixel_start + tile_y*pitch + (7-tile_x);
                uint8_t pal_index = (plane0 & 1) | ((plane1 & 1) << 1);
                uint32_t color = get_color_from_palette(selected_palette, pal_index);
                pattern_tables[table].pixels[i] = color;
                plane0 >>= 1; plane1 >>= 1;
            }
        }
    }
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
            ppu.nmi_occured = false;
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
            ppu.data_buffer = ppu_bus_read(ppu.vram_addr.reg);
            if (ppu.vram_addr.reg > 0x3EFF) data = ppu.data_buffer;
            ppu.vram_addr.reg += (CTRL_INC_VERTICAL ? 32 : 1);
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
        {
            ppu.registers[PPUCTRL] = data;
            ppu.vram_temp.bits.nt_select = data & 3;
            break;
        }
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
            if (ppu.offset_toggle) {
                ppu.vram_temp.bits.coarse_y = (data >> 3) & 0x1F;
                ppu.vram_temp.bits.fine_y = data & 7;
            } else {
                ppu.vram_temp.bits.coarse_x = (data >> 3) & 0x1F;
                ppu.fine_x = data & 7;
            }
            ppu.offset_toggle = !ppu.offset_toggle;
            break;
        }
        case PPUADDR:
        {
            if (ppu.offset_toggle) {
                ppu.vram_temp.reg = (ppu.vram_temp.reg & 0xFF00) | data;
                ppu.vram_addr.reg = ppu.vram_temp.reg;
            } else {
                /* NOTE(shaw): might need to cast data to uint16_t */
                ppu.vram_temp.reg = (ppu.vram_temp.reg & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);
            }

            ppu.offset_toggle = !ppu.offset_toggle;
            break;
        } 
        case PPUDATA:
            ppu_bus_write(ppu.vram_addr.reg, data);
            ppu.vram_addr.reg += (CTRL_INC_VERTICAL ? 32 : 1);
            break;
        default:
            break;
    }
}


/*
 *  PPU Addresses within the pattern tables
 *  0HRRRR CCCCPTTT
 *  |||||| |||||+++- T: Fine Y offset, the row number within a tile
 *  |||||| ||||+---- P: Bit plane (0: "lower"; 1: "upper")
 *  |||||| ++++----- C: Tile column
 *  ||++++---------- R: Tile row
 *  |+-------------- H: Half of sprite table (0: "left"; 1: "right")
 *  +--------------- 0: Pattern table is at $0000-$1FFF
 */
void rendering_tick(void) {
    if (ppu.cycle > 0 && (ppu.cycle < 256 || ppu.cycle > 320)) {
        ppu.bg_shifter_pat_lo <<= 1; ppu.bg_shifter_pat_hi <<= 1;
        ppu.bg_shifter_attr_lo <<= 1; ppu.bg_shifter_attr_hi <<= 1;
        loopy_t *v = &ppu.vram_addr;
        switch ((ppu.cycle - 1) % 8) {
        case 0:
            /* nametable fetch */
            ppu.nt_byte = ppu_bus_read(0x2000 | (v->reg & 0x0FFF));
            break;

        case 2:
        {
            /* attribute fetch */
            uint8_t at_byte = ppu_bus_read(0x23C0 | (v->reg & 0x0C00) | 
                ((v->reg >> 4) & 0x38) | ((v->reg >> 2) & 0x7));
            ppu.bg_shifter_attr_lo = (ppu.bg_shifter_attr_lo & 0xFF00) | ((at_byte & 1) ? 0xFF : 0);
            ppu.bg_shifter_attr_hi = (ppu.bg_shifter_attr_hi & 0xFF00) | ((at_byte & 2) ? 0xFF : 0);
            break;
        }

        case 4:
        {
            /* low bg tile byte */
            uint8_t bg_tile_lo = ppu_bus_read(
                (((uint16_t)CTRL_BG_TABLE) << 12) |
                (((int16_t)ppu.nt_byte) << 4) | 
                v->bits.fine_y);
            ppu.bg_shifter_pat_lo = (ppu.bg_shifter_pat_lo & 0xFF00) | bg_tile_lo;
            break;
        }

        case 6:
        {
            /* high bg tile byte */
            uint8_t bg_tile_hi = ppu_bus_read(
                (((uint16_t)CTRL_BG_TABLE) << 12) |
                (((int16_t)ppu.nt_byte) << 4) | 
                0x8 |
                v->bits.fine_y);
            ppu.bg_shifter_pat_hi = (ppu.bg_shifter_pat_hi & 0xFF00) | bg_tile_hi;
            break;
        }

        case 7:
            /* inc v horizontal */
            if (v->bits.coarse_x == 31) {
                v->bits.coarse_x = 0;
                v->bits.nt_select ^= 1;
            } else {
                ++v->bits.coarse_x;
            }
            break;
        }

    } else if (ppu.cycle == 256) {
        /* inc v vertical */
        loopy_t *v = &ppu.vram_addr;
        if (v->bits.fine_y < 7) {
            ++v->bits.fine_y;
        } else {
            v->bits.fine_y = 0;
            uint16_t coarse_y = v->bits.coarse_y;
            if (coarse_y == 29) {
                coarse_y = 0;
                v->bits.nt_select ^= 2;
            } else if (coarse_y == 31) {
                coarse_y = 0;
            } else {
                ++coarse_y;
                v->bits.coarse_y = coarse_y;
            }
        }

    } else if (ppu.cycle == 257) {
        /* copy horizontal data from loopy t to v */
        loopy_t *v = &ppu.vram_addr, t = ppu.vram_temp;
        v->bits.coarse_x = t.bits.coarse_x;
        v->bits.nt_select = (v->bits.nt_select & 2) | (t.bits.nt_select & 1);
    }
}

/*
 *  Indices into palette
 *  43210
 *  |||||
 *  |||++- Pixel value from tile data
 *  |++--- Palette number from attribute table or OAM
 *  +----- Background/Sprite select
 */
void render_pixel(void) {
    uint8_t bitnum = 15 - ppu.fine_x;

    uint8_t pal_index = 
        ((ppu.bg_shifter_pat_lo >> bitnum) & 1) |
        (((ppu.bg_shifter_pat_hi >> bitnum) & 1) << 1);

    uint8_t pal_num = 
        (((ppu.bg_shifter_attr_lo >> bitnum) & 1) << 2) |
        (((ppu.bg_shifter_attr_hi >> bitnum) & 1) << 3);

    uint32_t color = get_color_from_palette(pal_num, pal_index);
    ppu.screen_pixels[ppu.scanline * NES_WIDTH + ppu.cycle] = color;


    /*uint8_t val = rand() % 256;*/
    /*ppu.screen_pixels[ppu.scanline * NES_WIDTH + ppu.cycle] = (val << 16) | (val << 8) | val;*/
}

void ppu_tick(void) {
    if (ppu.scanline < 240) { 
        if (MASK_SHOW_SPR || MASK_SHOW_BG) rendering_tick();
        if (ppu.cycle < 256) render_pixel();
    } else if (ppu.scanline == 241 && ppu.cycle == 1) {
        ppu.registers[PPUSTATUS] |= 0x80;      /* set vblank */
        ppu.nmi_occured = true;
    } else if (ppu.scanline == 261) {
        if (ppu.cycle == 1) {
            ppu.registers[PPUSTATUS] &= ~0x80; /* clear vblank */
            ppu.nmi_occured = false;

        } else if (ppu.cycle > 279 && ppu.cycle < 305) {
            if (MASK_SHOW_SPR || MASK_SHOW_BG) {
                /* copy vertical data from loopy t to v */
                loopy_t *v = &ppu.vram_addr, t = ppu.vram_temp;
                v->bits.fine_y    = t.bits.fine_y;
                v->bits.coarse_y  = t.bits.coarse_y;
                v->bits.nt_select = (v->bits.nt_select & 1) | (t.bits.nt_select & 2);
            }

        } else if (ppu.cycle == 339 && ppu.even_odd_toggle && MASK_SHOW_BG) {
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
    return ppu.nmi_occured && CTRL_NMI;
}

void ppu_clear_nmi(void) {
    ppu.nmi_occured = false;
}

uint16_t ppu_get_cycle(void) {
    return ppu.cycle;
}

uint16_t ppu_get_scanline(void) {
    return ppu.scanline;
}


