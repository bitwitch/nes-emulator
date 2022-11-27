#ifndef __APU_H__
#define __APU_H__

#include <stdint.h>
#include <stdbool.h>

void apu_init(void);
uint8_t apu_read(uint16_t addr);
void apu_write(uint16_t addr, uint8_t data);
void apu_tick(void);
void apu_render_sound_wave(void);
bool apu_request_frame(void);
void apu_flush_sound_buffer(void);

#endif
