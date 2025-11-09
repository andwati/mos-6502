#include "cpu.h"
#include "memory.h"

// CPU Functions
void cpu_init(CPU* cpu) {
    // Initialize registers
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0xFF;    // Stack pointer starts at 0xFF
    cpu->P = FLAG_UNUSED;  // Status flags initialized with unused bit set
    cpu->PC = 0;       // Program counter starts at 0
    cpu->cycles = 0;
}

// Status flag operations
bool cpu_get_flag(CPU* cpu, uint8_t flag) {
    return (cpu->P & flag) != 0;
}

void cpu_set_flag(CPU* cpu, uint8_t flag, bool value) {
    if (value) {
        cpu->P |= flag;
    } else {
        cpu->P &= ~flag;
    }
}
