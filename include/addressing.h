#ifndef ADDRESSING_H
#define ADDRESSING_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint8_t (*read8)(uint16_t addr, void *userdata);
    void (*read16)(uint16_t addr, void *userdata);

    void *userdata;

    // cpu registers

} addr_ctx_t;

#endif