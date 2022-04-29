#ifndef __IO_H__
#define __IO_H__

#define WIDTH  256
#define HEIGHT 240

uint32_t *io_init(void);
void io_deinit(void);
void draw(void);
uint64_t get_ticks(void);
void do_input(void);

#endif
