#ifndef _CART_H_
#define _CART_H_

void read_rom_file(char *filepath);
void delete_cart();

uint8_t cart_cpu_read(uint16_t addr);
void cart_cpu_write(uint16_t addr, uint8_t data);
uint8_t cart_ppu_read(uint16_t addr, uint8_t vram[2048]);
void cart_ppu_write(uint16_t addr, uint8_t data, uint8_t vram[2048]);
void cart_scanline(void);
bool cart_irq_pending(void);
void cart_irq_clear(void);
void cart_scanline(void);

#endif
