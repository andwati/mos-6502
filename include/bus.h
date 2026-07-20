#ifndef BUS_H
#define BUS_H

#include <stdbool.h>
#include <stdint.h>
#include "memory.h"

typedef struct Bus6502 {
    Memory memory;
    uint8_t keyboard;
    uint32_t rng_state;
    bool easy6502_io;
    bool framebuffer_dirty;
} Bus6502;

void bus_init(Bus6502 *bus);
uint8_t bus_read(Bus6502 *bus, uint16_t addr);
void bus_write(Bus6502 *bus, uint16_t addr, uint8_t value);
uint16_t bus_read16(Bus6502 *bus, uint16_t addr);
void bus_write16(Bus6502 *bus, uint16_t addr, uint16_t value);
void bus_set_keyboard(Bus6502 *bus, uint8_t key);
void bus_seed_rng(Bus6502 *bus, uint32_t seed);

#endif
