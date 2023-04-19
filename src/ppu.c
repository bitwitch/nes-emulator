/*
 *  Loopy Registers:
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

/*
 *  OAM entry attributes:
 *  76543210
 *  ||||||||
 *  ||||||++- Palette (4 to 7) of sprite
 *  |||+++--- Unimplemented (read 0)
 *  ||+------ Priority (0: in front of background; 1: behind background)
 *  |+------- Flip sprite horizontally
 *  +-------- Flip sprite vertically
 */
#define SPR_ATTR_PRIORITY       (1 << 5)
#define SPR_ATTR_FLIP_HORIZ     (1 << 6)
#define SPR_ATTR_FLIP_VERT      (1 << 7)

typedef struct {
    uint8_t y, tile_id, attr, x;
} oam_entry_t;

typedef struct {
    uint8_t vram[2048];
    uint8_t palette_ram[32];
    uint8_t registers[9];
    uint8_t data_buffer;

    /* OAM format
    * byte 0: y position (top)
    * byte 1: tile_id
    * byte 2: attributes
    * byte 3: x position (left) 
    *
    * 64 sprites at 4 bytes each = 256 bytes
    */
    uint8_t oam[256];
    oam_entry_t oam2[8]; /* sprites on a single scanline */
    uint8_t oam_addr;
    bool sprite_overflow;
    uint8_t sprite_count_scanline;
    uint8_t spr_shifter_pat_lo[8];
    uint8_t spr_shifter_pat_hi[8];
    bool sprite_zero_hit_possible;

    uint32_t colors[64];
    uint32_t *screen_pixels;
    bool odd;
    bool frame_completed;
    bool nmi_occured;
    uint16_t cycle, scanline;

    loopy_t vram_addr, vram_temp;
    uint8_t fine_x;
    bool offset_toggle; /* used for writing low or high byte into address */

    uint8_t nt_byte, at_byte;
    uint8_t bg_tile_lo, bg_tile_hi; /* pattern table data fed to shifters */
    uint16_t bg_shifter_pat_lo;
    uint16_t bg_shifter_pat_hi;
    uint16_t bg_shifter_attr_lo;
    uint16_t bg_shifter_attr_hi;
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
	/* palette generated from http://drag.wootest.net/misc/palgen.html this format is ARGB */
	.colors = { 0x464646, 0x000154, 0x000070, 0x07006b, 0x280048, 0x3c000e, 0x3e0000, 0x2c0000, 0x0d0300, 0x001500, 0x001f00, 0x001f00, 0x001420, 0x000000, 0x000000, 0x000000, 0x9d9d9d, 0x0041b0, 0x1825d5, 0x4a0dcf, 0x75009f, 0x900153, 0x920f00, 0x7b2800, 0x514400, 0x205c00, 0x006900, 0x006916, 0x005a6a, 0x000000, 0x000000, 0x000000, 0xfeffff, 0x4896ff, 0x626dff, 0x8e5bff, 0xd45eff, 0xf160b4, 0xf36f5e, 0xdc8817, 0xb2a400, 0x7fbd00, 0x53ca28, 0x38ca76, 0x36bbcb, 0x2b2b2b, 0x000000, 0x000000, 0xfeffff, 0xb0d2ff, 0xb6bbff, 0xcbb4ff, 0xedbcff, 0xf9bde0, 0xfac3bd, 0xf0ce9f, 0xdfd990, 0xcae393, 0xb8e9a6, 0xade9c6, 0xace3e9, 0xa7a7a7, 0x000000, 0x000000 } };

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

#define STATUS_SPR_ZERO_HIT   ((ppu.registers[PPUSTATUS] >> 6) & 1)

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

/* A read on the internal PPU bus */
static uint8_t 
ppu_bus_read(uint16_t addr) {
    uint8_t data = 0;

    if (addr < 0x3F00) {
        data = cart_ppu_read(addr, ppu.vram);
    } else {
        addr = (addr - 0x3F00) & 0x1F;
        if (addr % 4 == 0) addr = 0;
        data = ppu.palette_ram[addr];
    }

    return data;
}

/* A write on the internal PPU bus */
static void 
ppu_bus_write(uint16_t addr, uint8_t data) {
    if (addr < 0x3F00) {
        cart_ppu_write(addr, data, ppu.vram);
    } else {
        addr = (addr - 0x3F00) & 0x1F;
        /* Addresses $3F04/$3F08/$3F0C can contain unique data */
        /* Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C */
        if (addr > 0x0F && addr % 4 == 0)
            addr -= 0x10;
        ppu.palette_ram[addr] = data;
    }
}

static uint32_t
get_color_from_palette(uint8_t palette_num, uint8_t palette_index) {
    uint16_t addr = palette_num*4 + palette_index;
    if (addr > 0x0F && addr % 4 == 0)
        addr -= 0x10;
    uint8_t index = ppu.palette_ram[addr];
    return ppu.colors[index];
}


static inline uint8_t 
reverse_byte(uint8_t b) {
    uint8_t result = 0;
    result |= (b & 0x80) >> 7;
    result |= (b & 0x40) >> 5;
    result |= (b & 0x20) >> 3;
    result |= (b & 0x10) >> 1;
    result |= (b & 0x08) << 1;
    result |= (b & 0x04) << 3;
    result |= (b & 0x02) << 5;
    result |= (b & 0x01) << 7;
    return result;
}


static void 
do_sprite_zero_hit(uint8_t fg_pal_index, uint8_t bg_pal_index) {
    if (ppu.sprite_zero_hit_possible &&
        fg_pal_index > 0 && bg_pal_index > 0 &&
        (ppu.cycle > 7 || (MASK_SHOW_BG_LEFT && MASK_SHOW_SPR_LEFT)) &&
        ppu.cycle > 1 &&
        ppu.cycle != 255)
    {
        ppu.registers[PPUSTATUS] |= 0x40; /* set sprite zero hit */
    }
}


void ppu_init(uint32_t *pixels) {
    ppu.screen_pixels = pixels;
    /* initialize with some random colors to visualize palette on load */
    /*for (int i=0; i<32; ++i) {*/
        /*int r = rand();*/
        /*ppu.palette_ram[i] = (uint8_t)(r >> 6);*/
        /*[>ppu.palette_ram[i] = (uint8_t)pixels[i];<]*/
    /*}*/
}


/* used just for debug drawing palettes */
void update_palettes(sprite_t palettes[8]) {
    for (int i=0; i<8; ++i) {
        uint32_t *pixels = palettes[i].pixels;
        for (int p=0; p<4; ++p) {
            pixels[p] = get_color_from_palette(i, p);
        }
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
	SDL_Surface *surface = pattern_tables[0].surface;
	assert(surface->format->BytesPerPixel);
    int pitch = surface->pitch / surface->format->BytesPerPixel;
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

/* evaluate up to 8 sprites for the next scanline */
void ppu_evaluate_sprites(void) {
    /* TODO(shaw): secondary OAM clear and sprite evaluation do not occur on
     * the pre-render scanline 261, but sprite tile fetches still do */
    if (ppu.scanline == 261) return;

    int i, sprite_row;
    oam_entry_t *sprite;
    int sprite_height = CTRL_EIGHT_BY_SIXTEEN ? 16 : 8;

    /* initialize secondary OAM */
    memset(ppu.oam2, 0xFF, 8*sizeof(oam_entry_t));
    ppu.sprite_count_scanline = 0;

    /* NOTE(shaw): does this get reset per scanline?? probably */
    ppu.sprite_overflow = false;
    ppu.sprite_zero_hit_possible = false;

    for (i=0; i<256; i+=4) {
        sprite = (oam_entry_t*)(ppu.oam+i);
        /* NOTE(shaw): why isn't this scanline + 1 for the next scanline???  if
         * the current scanline is used it is rendered correctly, but I don't
         * understand why?  If for example the scanline is 7 and the sprite y
         * position is 0, then the sprite should not appear on the next
         * scanline. If current scanline is used though, then it will
         */
        sprite_row = ppu.scanline - sprite->y;
        if (ppu.sprite_count_scanline < 9 && sprite_row >= 0 && sprite_row < sprite_height) {
            if (ppu.sprite_count_scanline == 8) {
                ppu.sprite_overflow = true;
                break;
            }

            if (i == 0)
                ppu.sprite_zero_hit_possible = true;

            /* vertical flip */
            if (sprite->attr & SPR_ATTR_FLIP_VERT) {
                sprite_row = sprite_height-1 - sprite_row;
            }

            /* default to 8x8 mode */
            uint8_t pattern_table = CTRL_SPRITE_TABLE;
            uint8_t tile_index = sprite->tile_id;

            /* 8x16 mode */
            if (sprite_height == 16) {
                pattern_table = sprite->tile_id & 1;
                tile_index = sprite->tile_id & 0xFE;
                if (sprite_row > 7) ++tile_index;
                sprite_row &= 0x07; /* map the overall row into just the 8x8 subtile */
            }

            uint16_t sprite_addr_lo = 
                (pattern_table << 12) |  /* which pattern table */
                tile_index * 16       |  /* offset into that pattern table */
                sprite_row;              /* offset into that tile */
                    
            uint8_t sprite_data_lo = ppu_bus_read(sprite_addr_lo);
            uint8_t sprite_data_hi = ppu_bus_read(sprite_addr_lo+8);

            /* horizontal flip */
            if (sprite->attr & SPR_ATTR_FLIP_HORIZ) {
                sprite_data_lo = reverse_byte(sprite_data_lo);
                sprite_data_hi = reverse_byte(sprite_data_hi);
            }

            ppu.spr_shifter_pat_lo[ppu.sprite_count_scanline] = sprite_data_lo;
            ppu.spr_shifter_pat_hi[ppu.sprite_count_scanline] = sprite_data_hi;

            /* add sprite to secondary OAM */
            memcpy(ppu.oam2+ppu.sprite_count_scanline, ppu.oam+i, sizeof(ppu.oam2[0]));
            ++ppu.sprite_count_scanline;
        }
    }
}

uint8_t ppu_read(uint16_t addr) {
    assert(addr <= 7);
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
            data = ppu.oam[ppu.oam_addr];
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


void ppu_write(uint16_t addr, uint8_t data) {
    assert(addr <= 7);
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
            ppu.oam_addr = data;
            break;
        case OAMDATA:
            ppu.oam[ppu.oam_addr++] = data;
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
                ppu.vram_temp.reg = (ppu.vram_temp.reg & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);

				// blargg says: mapper scanline counter can be clocked manually
				// via bit 12 of the VRAM address even when $2000 = $00 (bg and
				// sprites both use tiles from $0xxx).
				// however if i do this, it seems to break graphics in mapper 4
				// games like smb3
				// if (data & 0x08) cart_scanline();
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
    /* load low 8 bits of bg shift registers */
    if ((ppu.cycle > 1 && ppu.cycle < 258) || (ppu.cycle > 321 && ppu.cycle < 338)) {
        if (MASK_SHOW_BG) {
            ppu.bg_shifter_pat_lo <<= 1; ppu.bg_shifter_pat_hi <<= 1;
            ppu.bg_shifter_attr_lo <<= 1; ppu.bg_shifter_attr_hi <<= 1;
        }
    }

    if (ppu.cycle > 0 && (ppu.cycle < 256 || ppu.cycle > 320) && ppu.cycle < 337) {
        if (MASK_SHOW_SPR && ppu.cycle < 256) {
            oam_entry_t *sprite;
            for (int i=0; i<ppu.sprite_count_scanline; ++i) {
                sprite = ppu.oam2+i;
                if (sprite->x > 0) {
                    --sprite->x;
                } else {
                    ppu.spr_shifter_pat_lo[i] <<= 1;
                    ppu.spr_shifter_pat_hi[i] <<= 1;
                }
            }
        }

        loopy_t *v = &ppu.vram_addr;
        switch ((ppu.cycle - 1) % 8) {
        case 0:
            /* load current tile into shifters */
            ppu.bg_shifter_attr_lo = (ppu.bg_shifter_attr_lo & 0xFF00) | ((ppu.at_byte & 1) ? 0xFF : 0);
            ppu.bg_shifter_attr_hi = (ppu.bg_shifter_attr_hi & 0xFF00) | ((ppu.at_byte & 2) ? 0xFF : 0);
            ppu.bg_shifter_pat_lo = (ppu.bg_shifter_pat_lo & 0xFF00) | ppu.bg_tile_lo;
            ppu.bg_shifter_pat_hi = (ppu.bg_shifter_pat_hi & 0xFF00) | ppu.bg_tile_hi;

            /* nametable fetch */
            ppu.nt_byte = ppu_bus_read(0x2000 | (v->reg & 0x0FFF));
            break;

        case 2:
        {
            /* attribute fetch */
            ppu.at_byte = ppu_bus_read(
                0x23C0 |                    /* start of attribute table */
                (v->reg & 0x0C00) |         /* nametable select */
                ((v->reg >> 4) & 0x38) |    /* 3 high bits of coarse y */
                ((v->reg >> 2) & 0x7));     /* 3 high bits of coarse x */

            /* select the corresponding tile within the 2x2 tile square fetched from attribute table */
            if (v->bits.coarse_y & 2) ppu.at_byte >>= 4;
            if (v->bits.coarse_x & 2) ppu.at_byte >>= 2;
            ppu.at_byte &= 3;

            break;
        }

        case 4:
        {
            /* low bg tile byte */
            ppu.bg_tile_lo = ppu_bus_read(
                (((uint16_t)CTRL_BG_TABLE) << 12) |
                (((int16_t)ppu.nt_byte) << 4) | 
                v->bits.fine_y);
            break;
        }

        case 6:
        {
            /* high bg tile byte */
            ppu.bg_tile_hi = ppu_bus_read(
                (((uint16_t)CTRL_BG_TABLE) << 12) |
                (((int16_t)ppu.nt_byte) << 4) | 
                0x8 |
                v->bits.fine_y);
            break;
        }

        case 7:
            /* inc v horizontal, switch nametable if wraps */
            if (++v->bits.coarse_x == 0)
                v->bits.nt_select ^= 1;
            break;

        default:
            break;
        }


    } else if (ppu.cycle == 256) {
        /* inc v vertical */
        loopy_t *v = &ppu.vram_addr;

        if (++v->bits.fine_y == 0) {
            if (++v->bits.coarse_y == 30) {
                v->bits.coarse_y = 0;
                v->bits.nt_select ^= 2;
            }
        }

    } else if (ppu.cycle == 257) {
        /* copy horizontal data from loopy t to v */
        loopy_t *v = &ppu.vram_addr, t = ppu.vram_temp;
        v->bits.coarse_x = t.bits.coarse_x;
        v->bits.nt_select = (v->bits.nt_select & 2) | (t.bits.nt_select & 1);

        ppu_evaluate_sprites();

    } else if (ppu.cycle == 337 || ppu.cycle == 339) {
        /* unused nametable fetch */
        loopy_t *v = &ppu.vram_addr;
        ppu.nt_byte = ppu_bus_read(0x2000 | (v->reg & 0x0FFF));
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
    uint8_t bg_pal_index = 0;
    uint8_t bg_pal_num = 0;

    if (MASK_SHOW_BG) {
        uint8_t bitnum = 15 - ppu.fine_x;

        bg_pal_index = 
            ((ppu.bg_shifter_pat_lo >> bitnum) & 1) |
            (((ppu.bg_shifter_pat_hi >> bitnum) & 1) << 1);

        bg_pal_num = 
            ((ppu.bg_shifter_attr_lo >> bitnum) & 1) |
            (((ppu.bg_shifter_attr_hi >> bitnum) & 1) << 1);
    }

    if (ppu.cycle < 8 && !MASK_SHOW_BG_LEFT) {
        bg_pal_index = 0;
        bg_pal_num = 0;
    }

    uint8_t fg_pal_index = 0;
    uint8_t fg_pal_num   = 0;
    uint8_t fg_priority  = 0;
    if (MASK_SHOW_SPR) { 
        oam_entry_t *sprite;
        for (int i=0; i<ppu.sprite_count_scanline; ++i) {
            sprite = ppu.oam2+i;
            
            if (sprite->x == 0) {
                uint8_t lo = (ppu.spr_shifter_pat_lo[i] >> 7) & 1;
                uint8_t hi = (ppu.spr_shifter_pat_hi[i] >> 7) & 1;
                fg_pal_index = (hi << 1) | lo;
                fg_pal_num = (sprite->attr & 0x3) + 4;
                fg_priority = (sprite->attr >> 5) & 1;
            }

            if (i == 0)
                do_sprite_zero_hit(fg_pal_index, bg_pal_index);

            if (fg_pal_index != 0)
                break;
        }

    }

    if (ppu.cycle < 8 && !MASK_SHOW_SPR_LEFT) {
        fg_pal_index = 0;
        fg_pal_num = 0;
    }

    uint8_t pal_index = 0;
    uint8_t pal_num = 0;

    if (fg_pal_index == 0 && bg_pal_index == 0) {
        pal_index = 0;
        pal_num = 0;
    } else if (fg_pal_index > 0 && (fg_priority == 0 || bg_pal_index == 0)) {
        pal_index = fg_pal_index;
        pal_num = fg_pal_num;
    } else {
        pal_index = bg_pal_index;
        pal_num = bg_pal_num;
    }

    uint32_t pixel = get_color_from_palette(pal_num, pal_index);
    ppu.screen_pixels[ppu.scanline * NES_WIDTH + ppu.cycle] = pixel;
}

void ppu_tick(void) {
    if (ppu.scanline < 240) {
        if (MASK_SHOW_SPR || MASK_SHOW_BG) rendering_tick();
        if (ppu.cycle < 256) render_pixel();
    } else if (ppu.scanline == 241 && ppu.cycle == 1) {
        ppu.registers[PPUSTATUS] |= 0x80;      /* set vblank */
        ppu.nmi_occured = true;

    /* pre-render line */
    } else if (ppu.scanline == 261) {
        if (MASK_SHOW_SPR || MASK_SHOW_BG) rendering_tick();

        if (ppu.cycle == 1) {
            ppu.registers[PPUSTATUS] &= ~0x80; /* clear vblank */
            ppu.registers[PPUSTATUS] &= ~0x40; /* clear sprite zero hit */
            ppu.nmi_occured = false;
            memset(ppu.spr_shifter_pat_lo, 0, 8*sizeof(ppu.spr_shifter_pat_lo[0]));
            memset(ppu.spr_shifter_pat_hi, 0, 8*sizeof(ppu.spr_shifter_pat_lo[0]));

        } else if (ppu.cycle > 279 && ppu.cycle < 305) {
            if (MASK_SHOW_SPR || MASK_SHOW_BG) {
                /* copy vertical data from loopy t to v */
                loopy_t *v = &ppu.vram_addr, t = ppu.vram_temp;
                v->bits.fine_y    = t.bits.fine_y;
                v->bits.coarse_y  = t.bits.coarse_y;
                v->bits.nt_select = (v->bits.nt_select & 1) | (t.bits.nt_select & 2);
            }

        } else if (ppu.cycle == 340 && ppu.odd && MASK_SHOW_BG) {
            /* skip cycle 0 idle on odd ticks when bg enabled */
            ppu.cycle = 1;
            ppu.scanline = 0;
            ppu.frame_completed = true;
            ppu.odd = !ppu.odd;
            return;
        }
    }

	if ((MASK_SHOW_SPR || MASK_SHOW_BG) && (ppu.scanline < 241 && ppu.cycle == 260)) {
		// NOTE(shaw): this allows mappers to keep track of ppu scanlines
		cart_scanline();
	}

    if (++ppu.cycle > 340) {
        ppu.cycle = 0;
        if (++ppu.scanline > 261) {
            ppu.scanline = 0;
            ppu.frame_completed = true;
            ppu.odd = !ppu.odd;
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

uint8_t *ppu_get_oam(void) {
    return ppu.oam;
}

void ppu_reset(void) {
	memset(ppu.registers, 0, sizeof(ppu.registers));
	ppu.odd = 0;
}


