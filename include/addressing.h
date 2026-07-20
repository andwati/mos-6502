#ifndef ADDRESSING_H
#define ADDRESSING_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    AM_IMP, AM_ACC, AM_IMM, AM_ZP, AM_ZPX, AM_ZPY, AM_ABS, AM_ABSX,
    AM_ABSY, AM_IND, AM_INDX, AM_INDY, AM_REL
} addr_mode_t;

typedef struct {
    uint16_t address;
    bool page_crossed;
} addr_result_t;

#endif
