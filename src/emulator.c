#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"
#include "memory.h"
#include "instructions.h"

typedef struct {
    CPU cpu;
    Memory memory;
    bool running;
} Emulator;

void emulator_init(Emulator* emu) {
    cpu_init(&emu->cpu);
    memory_init(&emu->memory);
    emu->running = false;
}

void emulator_reset(Emulator* emu) {
    // Reset CPU
    cpu_init(&emu->cpu);
    
    // Set Program Counter to the reset vector location (0xFFFC-0xFFFD)
    uint16_t reset_vector = (uint16_t)emu->memory.data[0xFFFD] << 8 | emu->memory.data[0xFFFC];
    emu->cpu.PC = reset_vector;
    
    emu->running = true;
}

void emulator_step(Emulator* emu) {
    if (!emu->running) {
        return;
    }

    // Fetch
    uint8_t opcode = memory_read(&emu->memory, emu->cpu.PC);
    emu->cpu.PC++;

    // Decode and Execute
    switch (opcode) {
        // LDA
        case 0xA9: // LDA Immediate
            ins_lda(&emu->cpu, &emu->memory, addr_immediate(&emu->cpu, &emu->memory));
            emu->cpu.cycles += 2;
            break;
        case 0xA5: // LDA Zero Page
            ins_lda(&emu->cpu, &emu->memory, addr_zeropage(&emu->cpu, &emu->memory));
            emu->cpu.cycles += 3;
            break;
        case 0xB5: // LDA Zero Page,X
            ins_lda(&emu->cpu, &emu->memory, addr_zeropage_x(&emu->cpu, &emu->memory));
            emu->cpu.cycles += 4;
            break;
        case 0xAD: // LDA Absolute
            ins_lda(&emu->cpu, &emu->memory, addr_absolute(&emu->cpu, &emu->memory));
            emu->cpu.cycles += 4;
            break;

        // STA
        case 0x85: // STA Zero Page
            ins_sta(&emu->cpu, &emu->memory, addr_zeropage(&emu->cpu, &emu->memory));
            emu->cpu.cycles += 3;
            break;
        case 0x95: // STA Zero Page,X
            ins_sta(&emu->cpu, &emu->memory, addr_zeropage_x(&emu->cpu, &emu->memory));
            emu->cpu.cycles += 4;
            break;
        case 0x8D: // STA Absolute
            ins_sta(&emu->cpu, &emu->memory, addr_absolute(&emu->cpu, &emu->memory));
            emu->cpu.cycles += 4;
            break;

        // Register Transfers
        case 0xAA: // TAX
            ins_tax(&emu->cpu);
            emu->cpu.cycles += 2;
            break;
        case 0xA8: // TAY
            ins_tay(&emu->cpu);
            emu->cpu.cycles += 2;
            break;
        case 0x8A: // TXA
            ins_txa(&emu->cpu);
            emu->cpu.cycles += 2;
            break;
        case 0x98: // TYA
            ins_tya(&emu->cpu);
            emu->cpu.cycles += 2;
            break;

        // Stack Operations
        case 0x48: // PHA
            ins_pha(&emu->cpu, &emu->memory);
            emu->cpu.cycles += 3;
            break;
        case 0x08: // PHP
            ins_php(&emu->cpu, &emu->memory);
            emu->cpu.cycles += 3;
            break;
        case 0x68: // PLA
            ins_pla(&emu->cpu, &emu->memory);
            emu->cpu.cycles += 4;
            break;
        case 0x28: // PLP
            ins_plp(&emu->cpu, &emu->memory);
            emu->cpu.cycles += 4;
            break;
    }
}

void emulator_run(Emulator* emu) {
    emu->running = true;
    while (emu->running) {
        emulator_step(emu);
    }
}
