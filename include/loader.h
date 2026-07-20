#ifndef LOADER_H
#define LOADER_H

#include <stddef.h>
#include <stdint.h>
#include "bus.h"

int load_binary(Bus6502 *bus, const char *path, uint16_t address,
                size_t *bytes_loaded);

#endif
