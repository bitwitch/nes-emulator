#include <assert.h>
#include <stdint.h>
#include <string.h>

#define MAX_MEMORY 65536

static uint8_t memory[MAX_MEMORY];

void init_memory(void) {
    /*uint16_t addr = 0;*/
    /*memory[addr++] = 0x69; [> ADC #$1 <]*/
    /*memory[addr++] = 0x01;*/
    /*memory[addr++] = 0x69; [> ADC #$1 <]*/
    /*memory[addr++] = 0x01;*/
    /*memory[addr++] = 0x00; [> BRK <]*/
}

void load_memory(uint16_t addr, uint8_t *data, uint16_t size) {
    assert(addr + size <= MAX_MEMORY);
    memcpy(&memory[addr], data, size);
}

uint8_t bus_read(uint16_t addr) {
    return memory[addr];
}

void bus_write(uint16_t addr, uint8_t data) {
    memory[addr] = data;
}





