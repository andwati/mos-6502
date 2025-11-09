#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define MEMORY_SIZE 65536

typedef struct {
    uint8_t data[MEMORY_SIZE];
} Memory;

void memory_init(Memory* memory);
uint8_t memory_read(Memory* memory, uint16_t address);
void memory_write(Memory* memory, uint16_t address, uint8_t value);
void memory_write_bytes(Memory* memory, uint16_t address, const uint8_t* data, uint16_t length);
uint8_t memory_stack_pull(Memory* memory, uint8_t* sp);
void memory_stack_push(Memory* memory, uint8_t* sp, uint8_t value);

#endif // MEMORY_H