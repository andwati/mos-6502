#include <stdio.h>
#include "cpu.h"
#include "memory.h"
int main()
{
    CPU6502 cpu;
    Memory mem;

    mem_reset(&mem);
    cpu_reset(&cpu);

    cpu_print_state(&cpu);

    return 0;
}