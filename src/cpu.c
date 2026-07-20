#include <stdio.h>
#include <string.h>
#include "bus.h"
#include "cpu.h"

void cpu_reset(CPU6502 *cpu, Bus6502 *bus)
{
    memset(cpu, 0, sizeof(*cpu));
    cpu->SP = 0xFD;
    cpu->P = FLAG_U | FLAG_I;
    cpu->decimal_enabled = true;
    cpu->PC = bus_read16(bus, 0xFFFC);
}

void cpu_request_nmi(CPU6502 *cpu) { cpu->nmi_pending = true; }
void cpu_set_irq(CPU6502 *cpu, bool asserted) { cpu->irq_asserted = asserted; }

void cpu_print_state(const CPU6502 *cpu)
{
    printf("PC:%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
           cpu->PC, cpu->A, cpu->X, cpu->Y, cpu->P, cpu->SP);
}
