#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Bus6502 Bus6502;

typedef struct CPU6502 {
    uint8_t A, X, Y;
    uint8_t SP;
    uint16_t PC;
    uint8_t P;
    bool nmi_pending;
    bool irq_asserted;
} CPU6502;

enum {
    FLAG_C = 0x01, FLAG_Z = 0x02, FLAG_I = 0x04, FLAG_D = 0x08,
    FLAG_B = 0x10, FLAG_U = 0x20, FLAG_V = 0x40, FLAG_N = 0x80
};

typedef enum { CPU_STEP_OK = 0, CPU_STEP_ILLEGAL_OPCODE } cpu_step_status_t;

typedef struct {
    cpu_step_status_t status;
    uint8_t opcode;
    uint8_t cycles;
} cpu_step_result_t;

void cpu_reset(CPU6502 *cpu, Bus6502 *bus);
cpu_step_result_t cpu_step(CPU6502 *cpu, Bus6502 *bus);
void cpu_request_nmi(CPU6502 *cpu);
void cpu_set_irq(CPU6502 *cpu, bool asserted);
void cpu_print_state(const CPU6502 *cpu);

#endif
