#ifndef FRONTEND_H
#define FRONTEND_H
#include "bus.h"
#include "cpu.h"
int frontend_run(CPU6502 *cpu, Bus6502 *bus, unsigned hz, unsigned scale);
#endif
