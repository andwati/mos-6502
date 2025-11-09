#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "cpu.h"
#include "memory.h"

// Addressing Modes
uint16_t addr_immediate(CPU* cpu, Memory* memory);
uint16_t addr_zeropage(CPU* cpu, Memory* memory);
uint16_t addr_zeropage_x(CPU* cpu, Memory* memory);
uint16_t addr_zeropage_y(CPU* cpu, Memory* memory);
uint16_t addr_absolute(CPU* cpu, Memory* memory);
uint16_t addr_absolute_x(CPU* cpu, Memory* memory);
uint16_t addr_absolute_y(CPU* cpu, Memory* memory);
uint16_t addr_indirect(CPU* cpu, Memory* memory);
uint16_t addr_indirect_x(CPU* cpu, Memory* memory);
uint16_t addr_indirect_y(CPU* cpu, Memory* memory);

// Instructions
void ins_lda(CPU* cpu, Memory* memory, uint16_t addr);
void ins_ldx(CPU* cpu, Memory* memory, uint16_t addr);
void ins_ldy(CPU* cpu, Memory* memory, uint16_t addr);
void ins_sta(CPU* cpu, Memory* memory, uint16_t addr);
void ins_stx(CPU* cpu, Memory* memory, uint16_t addr);
void ins_sty(CPU* cpu, Memory* memory, uint16_t addr);
void ins_tax(CPU* cpu);
void ins_tay(CPU* cpu);
void ins_txa(CPU* cpu);
void ins_tya(CPU* cpu);

// Stack operations
void ins_pha(CPU* cpu, Memory* memory);
void ins_php(CPU* cpu, Memory* memory);
void ins_pla(CPU* cpu, Memory* memory);
void ins_plp(CPU* cpu, Memory* memory);

#endif // INSTRUCTIONS_H