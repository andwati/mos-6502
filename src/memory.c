#include "memory.h"
#include <string.h>

uint8_t mem_read(Memory *mem, uint16_t addr)
{
    return mem->data[addr];
}

void mem_write(Memory *mem, uint16_t addr, uint8_t value)
{
    mem->data[addr] = value;
}

void mem_reset(Memory *mem)
{
    memset(mem->data, 0, MEM_SIZE);
}