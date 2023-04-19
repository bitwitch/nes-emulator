#ifndef _PPU_H
#define _PPU_H

void ppu_init(uint32_t *pixels);
uint8_t ppu_read(uint16_t addr);
void ppu_write(uint16_t addr, uint8_t data);
void ppu_tick(void);
bool ppu_frame_completed(void);
void ppu_clear_frame_completed(void);
bool ppu_nmi(void);
void ppu_clear_nmi(void);
void update_palettes(sprite_t palettes[8]);
void update_pattern_tables(int selected_palette, sprite_t pattern_tables[2]);
void ppu_reset(void);

/* for debug sidebar to render oam info */
uint8_t *ppu_get_oam(void);

#endif
