#include <stdint.h>
#include <string.h>

#define MEMORY_SIZE 65536  // 64K memory space

typedef struct {
    uint8_t data[MEMORY_SIZE];
} Memory;

void memory_init(Memory* memory) {
    // Initialize all memory to 0
    memset(memory->data, 0, MEMORY_SIZE);
}

uint8_t memory_read(Memory* memory, uint16_t address) {
    return memory->data[address];
}

void memory_write(Memory* memory, uint16_t address, uint8_t value) {
    memory->data[address] = value;
}

// Helper function to write multiple bytes to memory
void memory_write_bytes(Memory* memory, uint16_t address, const uint8_t* data, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        memory_write(memory, address + i, data[i]);
    }
}

// Stack operations (stack is located at 0x0100-0x01FF)
uint8_t memory_stack_pull(Memory* memory, uint8_t* sp) {
    (*sp)++;
    return memory_read(memory, 0x0100 | *sp);
}

void memory_stack_push(Memory* memory, uint8_t* sp, uint8_t value) {
    memory_write(memory, 0x0100 | *sp, value);
    (*sp)--;
}
