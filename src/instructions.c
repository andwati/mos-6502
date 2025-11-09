#include "instructions.h"

// Addressing Modes Implementation
uint16_t addr_immediate(CPU* cpu, Memory* memory) {
    return cpu->PC++;
}

uint16_t addr_zeropage(CPU* cpu, Memory* memory) {
    return memory_read(memory, cpu->PC++);
}

uint16_t addr_zeropage_x(CPU* cpu, Memory* memory) {
    uint8_t addr = memory_read(memory, cpu->PC++);
    return (uint8_t)(addr + cpu->X);
}

uint16_t addr_zeropage_y(CPU* cpu, Memory* memory) {
    uint8_t addr = memory_read(memory, cpu->PC++);
    return (uint8_t)(addr + cpu->Y);
}

uint16_t addr_absolute(CPU* cpu, Memory* memory) {
    uint16_t addr_low = memory_read(memory, cpu->PC++);
    uint16_t addr_high = memory_read(memory, cpu->PC++);
    return (addr_high << 8) | addr_low;
}

uint16_t addr_absolute_x(CPU* cpu, Memory* memory) {
    uint16_t base = addr_absolute(cpu, memory);
    return base + cpu->X;
}

uint16_t addr_absolute_y(CPU* cpu, Memory* memory) {
    uint16_t base = addr_absolute(cpu, memory);
    return base + cpu->Y;
}

uint16_t addr_indirect(CPU* cpu, Memory* memory) {
    uint16_t ptr = addr_absolute(cpu, memory);
    // Simulate the 6502 page boundary bug
    uint16_t page = ptr & 0xFF00;
    uint8_t low = memory_read(memory, ptr);
    uint8_t high = memory_read(memory, (ptr & 0xFF00) | ((ptr + 1) & 0xFF));
    return (high << 8) | low;
}

uint16_t addr_indirect_x(CPU* cpu, Memory* memory) {
    uint8_t zero_addr = memory_read(memory, cpu->PC++);
    uint8_t ptr = (uint8_t)(zero_addr + cpu->X);
    uint16_t low = memory_read(memory, ptr);
    uint16_t high = memory_read(memory, (uint8_t)(ptr + 1));
    return (high << 8) | low;
}

uint16_t addr_indirect_y(CPU* cpu, Memory* memory) {
    uint8_t zero_addr = memory_read(memory, cpu->PC++);
    uint16_t low = memory_read(memory, zero_addr);
    uint16_t high = memory_read(memory, (uint8_t)(zero_addr + 1));
    uint16_t base = (high << 8) | low;
    return base + cpu->Y;
}

// Load/Store Instructions
void ins_lda(CPU* cpu, Memory* memory, uint16_t addr) {
    cpu->A = memory_read(memory, addr);
    cpu_set_flag(cpu, FLAG_ZERO, cpu->A == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->A & 0x80) != 0);
}

void ins_ldx(CPU* cpu, Memory* memory, uint16_t addr) {
    cpu->X = memory_read(memory, addr);
    cpu_set_flag(cpu, FLAG_ZERO, cpu->X == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->X & 0x80) != 0);
}

void ins_ldy(CPU* cpu, Memory* memory, uint16_t addr) {
    cpu->Y = memory_read(memory, addr);
    cpu_set_flag(cpu, FLAG_ZERO, cpu->Y == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->Y & 0x80) != 0);
}

void ins_sta(CPU* cpu, Memory* memory, uint16_t addr) {
    memory_write(memory, addr, cpu->A);
}

void ins_stx(CPU* cpu, Memory* memory, uint16_t addr) {
    memory_write(memory, addr, cpu->X);
}

void ins_sty(CPU* cpu, Memory* memory, uint16_t addr) {
    memory_write(memory, addr, cpu->Y);
}

// Register Transfer Instructions
void ins_tax(CPU* cpu) {
    cpu->X = cpu->A;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->X == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->X & 0x80) != 0);
}

void ins_tay(CPU* cpu) {
    cpu->Y = cpu->A;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->Y == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->Y & 0x80) != 0);
}

void ins_txa(CPU* cpu) {
    cpu->A = cpu->X;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->A == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->A & 0x80) != 0);
}

void ins_tya(CPU* cpu) {
    cpu->A = cpu->Y;
    cpu_set_flag(cpu, FLAG_ZERO, cpu->A == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->A & 0x80) != 0);
}

// Stack Operations
void ins_pha(CPU* cpu, Memory* memory) {
    memory_stack_push(memory, &cpu->SP, cpu->A);
}

void ins_php(CPU* cpu, Memory* memory) {
    memory_stack_push(memory, &cpu->SP, cpu->P | FLAG_BREAK | FLAG_UNUSED);
}

void ins_pla(CPU* cpu, Memory* memory) {
    cpu->A = memory_stack_pull(memory, &cpu->SP);
    cpu_set_flag(cpu, FLAG_ZERO, cpu->A == 0);
    cpu_set_flag(cpu, FLAG_NEGATIVE, (cpu->A & 0x80) != 0);
}

void ins_plp(CPU* cpu, Memory* memory) {
    cpu->P = (memory_stack_pull(memory, &cpu->SP) & ~FLAG_BREAK) | FLAG_UNUSED;
}