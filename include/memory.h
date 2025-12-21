#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define MEM_SIZE 65536 // 64KB memory size

typedef struct
{
    uint8_t data[MEM_SIZE];
} Memory;

uint8_t mem_read(Memory *mem, uint16_t addr);
void mem_write(Memory *mem, uint16_t addr, uint8_t value);
void mem_reset(Memory *mem);

#endif