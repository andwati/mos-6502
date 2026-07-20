#include <string.h>
#include "bus.h"

void bus_init(Bus6502 *bus)
{
    memset(bus, 0, sizeof(*bus));
    bus->rng_state = 0x6502u;
}

static uint8_t random_byte(Bus6502 *bus)
{
    uint32_t x = bus->rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    bus->rng_state = x ? x : 0x6502u;
    return (uint8_t)x;
}

uint8_t bus_read(Bus6502 *bus, uint16_t addr)
{
    uint8_t value;
    if(bus->read_hook&&bus->read_hook(bus->hook_userdata,addr,&value))return value;
    if (bus->easy6502_io) {
        if (addr == 0x00FE) return random_byte(bus);
        if (addr == 0x00FF) return bus->keyboard;
    }
    return mem_read(&bus->memory, addr);
}

void bus_write(Bus6502 *bus, uint16_t addr, uint8_t value)
{
    if(bus->write_hook&&bus->write_hook(bus->hook_userdata,addr,value))return;
    mem_write(&bus->memory, addr, value);
    if (addr >= 0x0200 && addr <= 0x05FF) bus->framebuffer_dirty = true;
}

uint16_t bus_read16(Bus6502 *bus, uint16_t addr)
{
    uint8_t lo = bus_read(bus, addr);
    uint8_t hi = bus_read(bus, (uint16_t)(addr + 1));
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

void bus_write16(Bus6502 *bus, uint16_t addr, uint16_t value)
{
    bus_write(bus, addr, (uint8_t)value);
    bus_write(bus, (uint16_t)(addr + 1), (uint8_t)(value >> 8));
}

void bus_set_keyboard(Bus6502 *bus, uint8_t key) { bus->keyboard = key; }
void bus_seed_rng(Bus6502 *bus, uint32_t seed) { bus->rng_state = seed ? seed : 1; }
void bus_set_hooks(Bus6502 *bus,bus_read_hook_t read_hook,bus_write_hook_t write_hook,void *userdata)
{ bus->read_hook=read_hook;bus->write_hook=write_hook;bus->hook_userdata=userdata; }
