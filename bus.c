#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "bus.h"
#include "cart.h"
#include "ppu.h"

#define MAX_MEMORY 65536

static uint8_t cpu_ram[2048];


/*
    Memory map

    Adress Range    size    device
    ------------    -----   ------
    $0000-$07FF 	$0800 	2KB internal RAM
    $0800-$0FFF 	$0800 	Mirrors of $0000-$07FF
    $1000-$17FF 	$0800   ""
    $1800-$1FFF 	$0800   ""
    $2000-$2007 	$0008 	NES PPU registers
    $2008-$3FFF 	$1FF8 	Mirrors of $2000-2007 (repeats every 8 bytes)
    $4000-$4017 	$0018 	NES APU and I/O registers
    $4018-$401F 	$0008 	APU and I/O functionality that is normally disabled. See CPU Test Mode on nesdev.
    $4020-$FFFF 	$BFE0 	Cartridge space: PRG ROM, PRG RAM, and mapper registers 

    from: https://www.nesdev.org/wiki/CPU_memory_map
*/
uint8_t bus_read(uint16_t addr) {
    if (addr < 0x2000)
        return cpu_ram[addr & 0x7FF];
    else if (addr < 0x4000)
        return ppu_read((addr-0x2000)&0xF);
    else if (addr < 0x4020) {
        switch (addr) {
            case 0x4014: break;
            case 0x4015: 
                /* apu read */
                break;
            case 0x4016: 
                return controller_read(0);
            case 0x4017: 
                return controller_read(1);
            default: 
                break;
        }
    } 
    else 
        return cart_read(addr);
    return 0;
}

void bus_write(uint16_t addr, uint8_t data) {
    if (addr < 0x2000)
        cpu_ram[addr & 0x7FF] = data;
    else if (addr < 0x4000)
        ppu_write((addr-0x2000)&0xF, data);
    else if (addr < 0x4020) {
        switch (addr) {
        case 0x4014: 
        {
            /* OAMDMA: Copy 256 bytes from cpu ram into ppu sprite memory */
            for(uint16_t b=0; b<256; ++b) {
                /* NOTE(shaw): bisqwits emulator only uses low 3 bits??
                 * bus_read((data&7)*0x0100+b) */
                bus_write(0x2004, bus_read(0x100*data + b));
            }
            break;
        }
        case 0x4015: 
            /* apu write */
            break;
        case 0x4016: 
            /* controller 0 write */
            controller_write(0, data);
            break;
        case 0x4017: 
            /* controller 1 write */
            controller_write(1, data);
            break;
        default: 
            /* apu write */
            break;
        }
    } 
    else 
        cart_write(addr, data);
}

void system_reset(cpu_t *cpu) {
    /*ppu_reset();*/
    cpu_reset(cpu);
}

