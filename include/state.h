#ifndef STATE_H
#define STATE_H
#include "bus.h"
#include "cpu.h"
int state_save(const char *path,const CPU6502 *cpu,const Bus6502 *bus);
int state_load(const char *path,CPU6502 *cpu,Bus6502 *bus);
#endif
