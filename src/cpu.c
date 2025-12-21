#include <stdio.h>
#include "cpu.h"

void cpu_reset(CPU6502 *cpu)
{
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0xFD;   // typical 6502 reset value??
    cpu->P = FLAG_U;  // unused flag always 1
    cpu->PC = 0x0000; // loaded later from reset vector
}

void cpu_print_state(CPU6502 *cpu)
{
    printf("CPU State:\n");
    printf("A:  0x%02X\n", cpu->A);
    printf("X:  0x%02X\n", cpu->X);
    printf("Y:  0x%02X\n", cpu->Y);
    printf("SP: 0x%02X\n", cpu->SP);
    printf("PC: 0x%04X\n", cpu->PC);
    printf("P:  0x%02X\n", cpu->P);
}