#ifndef OPCODE_H
#define OPCODE_H

#include <stddef.h>
#include <stdint.h>
#include "bus.h"
#include "cpu.h"

size_t cpu_disassemble(Bus6502 *bus, uint16_t pc, char *output, size_t size);

#endif
