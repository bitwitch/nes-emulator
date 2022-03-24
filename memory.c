#include <assert.h>
#include <stdint.h>

static uint8_t memory[65536];

void init_memory(void) {
    uint16_t addr = 0;
    memory[addr++] = 0x69; /* ADC #$1 */
    memory[addr++] = 0x01;
    memory[addr++] = 0x69; /* ADC #$1 */
    memory[addr++] = 0x01;
    memory[addr++] = 0x00; /* BRK */
}

uint8_t read_address(uint16_t addr) {
    return addr;
    /*return memory[addr];*/
}

void write_address(uint16_t addr, uint8_t data) {
    (void)addr;
    (void)data;
    assert(0 && "not implemented");
}





