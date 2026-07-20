#ifndef BUS_H
#define BUS_H

#include <stdbool.h>
#include <stdint.h>
#include "memory.h"

typedef struct Bus6502 Bus6502;
typedef bool (*bus_read_hook_t)(void *userdata,uint16_t addr,uint8_t *value);
typedef bool (*bus_write_hook_t)(void *userdata,uint16_t addr,uint8_t value);

struct Bus6502 {
    Memory memory;
    uint8_t keyboard;
    uint32_t rng_state;
    bool easy6502_io;
    bool framebuffer_dirty;
    bus_read_hook_t read_hook;
    bus_write_hook_t write_hook;
    void *hook_userdata;
};

void bus_init(Bus6502 *bus);
uint8_t bus_read(Bus6502 *bus, uint16_t addr);
void bus_write(Bus6502 *bus, uint16_t addr, uint8_t value);
uint16_t bus_read16(Bus6502 *bus, uint16_t addr);
void bus_write16(Bus6502 *bus, uint16_t addr, uint16_t value);
void bus_set_keyboard(Bus6502 *bus, uint8_t key);
void bus_seed_rng(Bus6502 *bus, uint32_t seed);
void bus_set_hooks(Bus6502 *bus,bus_read_hook_t read_hook,
                   bus_write_hook_t write_hook,void *userdata);

#endif
