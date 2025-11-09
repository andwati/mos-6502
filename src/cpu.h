#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

// CPU Status Flags
#define FLAG_CARRY     (1 << 0)
#define FLAG_ZERO      (1 << 1)
#define FLAG_INTERRUPT (1 << 2)
#define FLAG_DECIMAL   (1 << 3)
#define FLAG_BREAK     (1 << 4)
#define FLAG_UNUSED    (1 << 5)
#define FLAG_OVERFLOW  (1 << 6)
#define FLAG_NEGATIVE  (1 << 7)

typedef struct {
    uint8_t A;    // Accumulator
    uint8_t X;    // X Index Register
    uint8_t Y;    // Y Index Register
    uint8_t SP;   // Stack Pointer
    uint8_t P;    // Status Register
    uint16_t PC;  // Program Counter
    uint64_t cycles;
} CPU;

void cpu_init(CPU* cpu);
bool cpu_get_flag(CPU* cpu, uint8_t flag);
void cpu_set_flag(CPU* cpu, uint8_t flag, bool value);

#endif // CPU_H