#include <assert.h>
#include <stdint.h>

/*uint8_t memory[65536];*/

void write_address(uint16_t addr, uint8_t data) {
    (void)addr;
    (void)data;
    assert(0 && "not implemented");
}


uint8_t read_address(uint16_t addr) {
    return addr;
    /*return memory[addr];*/
}




