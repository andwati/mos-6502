#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct
{
    uint8_t A;  // accumulator
    uint8_t X;  // index X
    uint8_t Y;  // index y
    uint8_t SP; // stack pointer
    uint8_t PC; // program counter
    uint8_t P;  // status flags
} CPU6502;

// status flag masks
#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08
#define FLAG_B 0x10
#define FLAG_U 0x20
#define FLAG_V 0x40
#define FLAG_N 0x80

void cpu_reset(CPU6502 *cpu);
void cpu_print_state(CPU6502 *cpu);

#endif