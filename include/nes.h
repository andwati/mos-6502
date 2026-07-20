#ifndef NES_H
#define NES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bus.h"
#include "cpu.h"

typedef struct {
    uint8_t prg[512*1024],chr[256*1024],ram[2048],prg_ram[8192];
    uint8_t nametable[4096],palette[32],oam[256];
    size_t prg_size,chr_size;uint8_t mapper,prg_bank,chr_bank,mirroring;bool chr_ram;
    uint8_t ppu_ctrl,ppu_mask,ppu_status,oam_addr,ppu_latch,read_buffer;
    uint8_t scroll_x,scroll_y;
    uint16_t ppu_addr;unsigned ppu_dot,ppu_scanline;bool frame_ready;
    uint8_t controller,controller_shift;bool controller_strobe;
    uint8_t apu_reg[0x18],apu_status;uint64_t apu_cycles;
    double pulse_phase[2],triangle_phase,noise_phase;uint16_t noise_lfsr;
    Bus6502 bus;CPU6502 cpu;
} NES;

int nes_load(NES *nes,const char *path);
void nes_reset(NES *nes);
cpu_step_result_t nes_step(NES *nes);
void nes_set_controller(NES *nes,uint8_t buttons);
void nes_render_frame(NES *nes,uint32_t pixels[256*240]);
float nes_audio_sample(NES *nes,double sample_rate);
#endif
