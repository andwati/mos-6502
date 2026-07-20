#ifndef APPLE1_H
#define APPLE1_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bus.h"
typedef struct {
    uint8_t key;bool key_ready;bool stdout_enabled;
    char output[8192];size_t output_length;
} Apple1IO;
void apple1_init(Apple1IO *io,Bus6502 *bus);
void apple1_key(Apple1IO *io,uint8_t ascii);
void apple1_enable_stdout(Apple1IO *io,bool enabled);
#endif
