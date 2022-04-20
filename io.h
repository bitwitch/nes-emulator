#ifndef __IO_H__
#define __IO_H__

#define WIDTH  256
#define HEIGHT 240

void draw(void);
uint32_t *io_init(void);
void io_deinit(void);

#endif
