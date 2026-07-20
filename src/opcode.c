#include <stdio.h>
#include "addressing.h"
#include "bus.h"
#include "cpu.h"

typedef enum {
    O_BAD, O_ADC, O_AND, O_ASL, O_BCC, O_BCS, O_BEQ, O_BIT, O_BMI,
    O_BNE, O_BPL, O_BRK, O_BVC, O_BVS, O_CLC, O_CLD, O_CLI, O_CLV,
    O_CMP, O_CPX, O_CPY, O_DEC, O_DEX, O_DEY, O_EOR, O_INC, O_INX,
    O_INY, O_JMP, O_JSR, O_LDA, O_LDX, O_LDY, O_LSR, O_NOP, O_ORA,
    O_PHA, O_PHP, O_PLA, O_PLP, O_ROL, O_ROR, O_RTI, O_RTS, O_SBC,
    O_SEC, O_SED, O_SEI, O_STA, O_STX, O_STY, O_TAX, O_TAY, O_TSX,
    O_TXA, O_TXS, O_TYA, O_LAX, O_SAX, O_DCP, O_ISC, O_SLO, O_RLA,
    O_SRE, O_RRA, O_ANC, O_ALR, O_ARR, O_AXS
} operation_t;

typedef struct { operation_t op; addr_mode_t mode; uint8_t cycles; bool page; } opcode_t;
#define D(code, op, mode, cyc, pg) [code] = { O_##op, AM_##mode, cyc, pg }
static const opcode_t ops[256] = {
    D(0x00,BRK,IMP,7,0), D(0x01,ORA,INDX,6,0), D(0x05,ORA,ZP,3,0), D(0x06,ASL,ZP,5,0),
    D(0x08,PHP,IMP,3,0), D(0x09,ORA,IMM,2,0), D(0x0A,ASL,ACC,2,0), D(0x0D,ORA,ABS,4,0), D(0x0E,ASL,ABS,6,0),
    D(0x10,BPL,REL,2,0), D(0x11,ORA,INDY,5,1), D(0x15,ORA,ZPX,4,0), D(0x16,ASL,ZPX,6,0),
    D(0x18,CLC,IMP,2,0), D(0x19,ORA,ABSY,4,1), D(0x1D,ORA,ABSX,4,1), D(0x1E,ASL,ABSX,7,0),
    D(0x20,JSR,ABS,6,0), D(0x21,AND,INDX,6,0), D(0x24,BIT,ZP,3,0), D(0x25,AND,ZP,3,0), D(0x26,ROL,ZP,5,0),
    D(0x28,PLP,IMP,4,0), D(0x29,AND,IMM,2,0), D(0x2A,ROL,ACC,2,0), D(0x2C,BIT,ABS,4,0), D(0x2D,AND,ABS,4,0), D(0x2E,ROL,ABS,6,0),
    D(0x30,BMI,REL,2,0), D(0x31,AND,INDY,5,1), D(0x35,AND,ZPX,4,0), D(0x36,ROL,ZPX,6,0),
    D(0x38,SEC,IMP,2,0), D(0x39,AND,ABSY,4,1), D(0x3D,AND,ABSX,4,1), D(0x3E,ROL,ABSX,7,0),
    D(0x40,RTI,IMP,6,0), D(0x41,EOR,INDX,6,0), D(0x45,EOR,ZP,3,0), D(0x46,LSR,ZP,5,0),
    D(0x48,PHA,IMP,3,0), D(0x49,EOR,IMM,2,0), D(0x4A,LSR,ACC,2,0), D(0x4C,JMP,ABS,3,0), D(0x4D,EOR,ABS,4,0), D(0x4E,LSR,ABS,6,0),
    D(0x50,BVC,REL,2,0), D(0x51,EOR,INDY,5,1), D(0x55,EOR,ZPX,4,0), D(0x56,LSR,ZPX,6,0),
    D(0x58,CLI,IMP,2,0), D(0x59,EOR,ABSY,4,1), D(0x5D,EOR,ABSX,4,1), D(0x5E,LSR,ABSX,7,0),
    D(0x60,RTS,IMP,6,0), D(0x61,ADC,INDX,6,0), D(0x65,ADC,ZP,3,0), D(0x66,ROR,ZP,5,0),
    D(0x68,PLA,IMP,4,0), D(0x69,ADC,IMM,2,0), D(0x6A,ROR,ACC,2,0), D(0x6C,JMP,IND,5,0), D(0x6D,ADC,ABS,4,0), D(0x6E,ROR,ABS,6,0),
    D(0x70,BVS,REL,2,0), D(0x71,ADC,INDY,5,1), D(0x75,ADC,ZPX,4,0), D(0x76,ROR,ZPX,6,0),
    D(0x78,SEI,IMP,2,0), D(0x79,ADC,ABSY,4,1), D(0x7D,ADC,ABSX,4,1), D(0x7E,ROR,ABSX,7,0),
    D(0x81,STA,INDX,6,0), D(0x84,STY,ZP,3,0), D(0x85,STA,ZP,3,0), D(0x86,STX,ZP,3,0), D(0x88,DEY,IMP,2,0),
    D(0x8A,TXA,IMP,2,0), D(0x8C,STY,ABS,4,0), D(0x8D,STA,ABS,4,0), D(0x8E,STX,ABS,4,0),
    D(0x90,BCC,REL,2,0), D(0x91,STA,INDY,6,0), D(0x94,STY,ZPX,4,0), D(0x95,STA,ZPX,4,0), D(0x96,STX,ZPY,4,0),
    D(0x98,TYA,IMP,2,0), D(0x99,STA,ABSY,5,0), D(0x9A,TXS,IMP,2,0), D(0x9D,STA,ABSX,5,0),
    D(0xA0,LDY,IMM,2,0), D(0xA1,LDA,INDX,6,0), D(0xA2,LDX,IMM,2,0), D(0xA4,LDY,ZP,3,0), D(0xA5,LDA,ZP,3,0), D(0xA6,LDX,ZP,3,0),
    D(0xA8,TAY,IMP,2,0), D(0xA9,LDA,IMM,2,0), D(0xAA,TAX,IMP,2,0), D(0xAC,LDY,ABS,4,0), D(0xAD,LDA,ABS,4,0), D(0xAE,LDX,ABS,4,0),
    D(0xB0,BCS,REL,2,0), D(0xB1,LDA,INDY,5,1), D(0xB4,LDY,ZPX,4,0), D(0xB5,LDA,ZPX,4,0), D(0xB6,LDX,ZPY,4,0),
    D(0xB8,CLV,IMP,2,0), D(0xB9,LDA,ABSY,4,1), D(0xBA,TSX,IMP,2,0), D(0xBC,LDY,ABSX,4,1), D(0xBD,LDA,ABSX,4,1), D(0xBE,LDX,ABSY,4,1),
    D(0xC0,CPY,IMM,2,0), D(0xC1,CMP,INDX,6,0), D(0xC4,CPY,ZP,3,0), D(0xC5,CMP,ZP,3,0), D(0xC6,DEC,ZP,5,0),
    D(0xC8,INY,IMP,2,0), D(0xC9,CMP,IMM,2,0), D(0xCA,DEX,IMP,2,0), D(0xCC,CPY,ABS,4,0), D(0xCD,CMP,ABS,4,0), D(0xCE,DEC,ABS,6,0),
    D(0xD0,BNE,REL,2,0), D(0xD1,CMP,INDY,5,1), D(0xD5,CMP,ZPX,4,0), D(0xD6,DEC,ZPX,6,0),
    D(0xD8,CLD,IMP,2,0), D(0xD9,CMP,ABSY,4,1), D(0xDD,CMP,ABSX,4,1), D(0xDE,DEC,ABSX,7,0),
    D(0xE0,CPX,IMM,2,0), D(0xE1,SBC,INDX,6,0), D(0xE4,CPX,ZP,3,0), D(0xE5,SBC,ZP,3,0), D(0xE6,INC,ZP,5,0),
    D(0xE8,INX,IMP,2,0), D(0xE9,SBC,IMM,2,0), D(0xEA,NOP,IMP,2,0), D(0xEC,CPX,ABS,4,0), D(0xED,SBC,ABS,4,0), D(0xEE,INC,ABS,6,0),
    D(0xF0,BEQ,REL,2,0), D(0xF1,SBC,INDY,5,1), D(0xF5,SBC,ZPX,4,0), D(0xF6,INC,ZPX,6,0),
    D(0xF8,SED,IMP,2,0), D(0xF9,SBC,ABSY,4,1), D(0xFD,SBC,ABSX,4,1), D(0xFE,INC,ABSX,7,0)
    ,D(0xA3,LAX,INDX,6,0),D(0xA7,LAX,ZP,3,0),D(0xAF,LAX,ABS,4,0),D(0xB3,LAX,INDY,5,1),D(0xB7,LAX,ZPY,4,0),D(0xBF,LAX,ABSY,4,1)
    ,D(0x83,SAX,INDX,6,0),D(0x87,SAX,ZP,3,0),D(0x8F,SAX,ABS,4,0),D(0x97,SAX,ZPY,4,0)
    ,D(0xC3,DCP,INDX,8,0),D(0xC7,DCP,ZP,5,0),D(0xCF,DCP,ABS,6,0),D(0xD3,DCP,INDY,8,0),D(0xD7,DCP,ZPX,6,0),D(0xDB,DCP,ABSY,7,0),D(0xDF,DCP,ABSX,7,0)
    ,D(0xE3,ISC,INDX,8,0),D(0xE7,ISC,ZP,5,0),D(0xEF,ISC,ABS,6,0),D(0xF3,ISC,INDY,8,0),D(0xF7,ISC,ZPX,6,0),D(0xFB,ISC,ABSY,7,0),D(0xFF,ISC,ABSX,7,0)
    ,D(0x03,SLO,INDX,8,0),D(0x07,SLO,ZP,5,0),D(0x0F,SLO,ABS,6,0),D(0x13,SLO,INDY,8,0),D(0x17,SLO,ZPX,6,0),D(0x1B,SLO,ABSY,7,0),D(0x1F,SLO,ABSX,7,0)
    ,D(0x23,RLA,INDX,8,0),D(0x27,RLA,ZP,5,0),D(0x2F,RLA,ABS,6,0),D(0x33,RLA,INDY,8,0),D(0x37,RLA,ZPX,6,0),D(0x3B,RLA,ABSY,7,0),D(0x3F,RLA,ABSX,7,0)
    ,D(0x43,SRE,INDX,8,0),D(0x47,SRE,ZP,5,0),D(0x4F,SRE,ABS,6,0),D(0x53,SRE,INDY,8,0),D(0x57,SRE,ZPX,6,0),D(0x5B,SRE,ABSY,7,0),D(0x5F,SRE,ABSX,7,0)
    ,D(0x63,RRA,INDX,8,0),D(0x67,RRA,ZP,5,0),D(0x6F,RRA,ABS,6,0),D(0x73,RRA,INDY,8,0),D(0x77,RRA,ZPX,6,0),D(0x7B,RRA,ABSY,7,0),D(0x7F,RRA,ABSX,7,0)
    ,D(0x0B,ANC,IMM,2,0),D(0x2B,ANC,IMM,2,0),D(0x4B,ALR,IMM,2,0),D(0x6B,ARR,IMM,2,0),D(0xCB,AXS,IMM,2,0),D(0xEB,SBC,IMM,2,0)
    ,D(0x1A,NOP,IMP,2,0),D(0x3A,NOP,IMP,2,0),D(0x5A,NOP,IMP,2,0),D(0x7A,NOP,IMP,2,0),D(0xDA,NOP,IMP,2,0),D(0xFA,NOP,IMP,2,0)
    ,D(0x80,NOP,IMM,2,0),D(0x82,NOP,IMM,2,0),D(0x89,NOP,IMM,2,0),D(0xC2,NOP,IMM,2,0),D(0xE2,NOP,IMM,2,0)
    ,D(0x04,NOP,ZP,3,0),D(0x44,NOP,ZP,3,0),D(0x64,NOP,ZP,3,0),D(0x14,NOP,ZPX,4,0),D(0x34,NOP,ZPX,4,0),D(0x54,NOP,ZPX,4,0),D(0x74,NOP,ZPX,4,0),D(0xD4,NOP,ZPX,4,0),D(0xF4,NOP,ZPX,4,0),D(0x0C,NOP,ABS,4,0),D(0x1C,NOP,ABSX,4,1),D(0x3C,NOP,ABSX,4,1),D(0x5C,NOP,ABSX,4,1),D(0x7C,NOP,ABSX,4,1),D(0xDC,NOP,ABSX,4,1),D(0xFC,NOP,ABSX,4,1)
};
#undef D

size_t cpu_disassemble(Bus6502 *b, uint16_t pc, char *out, size_t size)
{
    static const char *names[]={"???","ADC","AND","ASL","BCC","BCS","BEQ","BIT","BMI","BNE","BPL","BRK","BVC","BVS","CLC","CLD","CLI","CLV","CMP","CPX","CPY","DEC","DEX","DEY","EOR","INC","INX","INY","JMP","JSR","LDA","LDX","LDY","LSR","NOP","ORA","PHA","PHP","PLA","PLP","ROL","ROR","RTI","RTS","SBC","SEC","SED","SEI","STA","STX","STY","TAX","TAY","TSX","TXA","TXS","TYA","LAX","SAX","DCP","ISC","SLO","RLA","SRE","RRA","ANC","ALR","ARR","AXS"};
    opcode_t d=ops[bus_read(b,pc)];uint8_t a=bus_read(b,(uint16_t)(pc+1));uint8_t z=bus_read(b,(uint16_t)(pc+2));
    const char *name=names[d.op];size_t bytes=1;
    switch(d.mode){
    case AM_ACC:snprintf(out,size,"%s A",name);break;
    case AM_IMM:snprintf(out,size,"%s #$%02X",name,a);bytes=2;break;
    case AM_ZP:snprintf(out,size,"%s $%02X",name,a);bytes=2;break;
    case AM_ZPX:snprintf(out,size,"%s $%02X,X",name,a);bytes=2;break;
    case AM_ZPY:snprintf(out,size,"%s $%02X,Y",name,a);bytes=2;break;
    case AM_ABS:snprintf(out,size,"%s $%04X",name,(uint16_t)(a|z<<8));bytes=3;break;
    case AM_ABSX:snprintf(out,size,"%s $%04X,X",name,(uint16_t)(a|z<<8));bytes=3;break;
    case AM_ABSY:snprintf(out,size,"%s $%04X,Y",name,(uint16_t)(a|z<<8));bytes=3;break;
    case AM_IND:snprintf(out,size,"%s ($%04X)",name,(uint16_t)(a|z<<8));bytes=3;break;
    case AM_INDX:snprintf(out,size,"%s ($%02X,X)",name,a);bytes=2;break;
    case AM_INDY:snprintf(out,size,"%s ($%02X),Y",name,a);bytes=2;break;
    case AM_REL:snprintf(out,size,"%s $%04X",name,(uint16_t)(pc+2+(int8_t)a));bytes=2;break;
    default:snprintf(out,size,"%s",name);break;
    }
    return bytes;
}

static void set_flag(CPU6502 *c, uint8_t f, bool on) { if (on) c->P |= f; else c->P &= (uint8_t)~f; }
static void set_nz(CPU6502 *c, uint8_t v) { set_flag(c, FLAG_Z, v == 0); set_flag(c, FLAG_N, v & 0x80); }
static void push(CPU6502 *c, Bus6502 *b, uint8_t v) { bus_write(b, (uint16_t)(0x100 | c->SP--), v); }
static uint8_t pull(CPU6502 *c, Bus6502 *b) { return bus_read(b, (uint16_t)(0x100 | ++c->SP)); }

static addr_result_t address(CPU6502 *c, Bus6502 *b, addr_mode_t mode)
{
    addr_result_t r = {0, false}; uint16_t base; uint8_t zp;
    switch (mode) {
    case AM_IMM: r.address = c->PC++; break;
    case AM_ZP: r.address = bus_read(b, c->PC++); break;
    case AM_ZPX: r.address = (uint8_t)(bus_read(b, c->PC++) + c->X); break;
    case AM_ZPY: r.address = (uint8_t)(bus_read(b, c->PC++) + c->Y); break;
    case AM_ABS: r.address = bus_read16(b, c->PC); c->PC += 2; break;
    case AM_ABSX: base = bus_read16(b, c->PC); c->PC += 2; r.address = base + c->X; r.page_crossed = (base & 0xFF00) != (r.address & 0xFF00); break;
    case AM_ABSY: base = bus_read16(b, c->PC); c->PC += 2; r.address = base + c->Y; r.page_crossed = (base & 0xFF00) != (r.address & 0xFF00); break;
    case AM_IND: base = bus_read16(b, c->PC); c->PC += 2; r.address = (uint16_t)bus_read(b, base) | ((uint16_t)bus_read(b, (uint16_t)((base & 0xFF00) | (uint8_t)(base + 1))) << 8); break;
    case AM_INDX: zp = (uint8_t)(bus_read(b, c->PC++) + c->X); r.address = (uint16_t)bus_read(b, zp) | ((uint16_t)bus_read(b, (uint8_t)(zp + 1)) << 8); break;
    case AM_INDY: zp = bus_read(b, c->PC++); base = (uint16_t)bus_read(b, zp) | ((uint16_t)bus_read(b, (uint8_t)(zp + 1)) << 8); r.address = base + c->Y; r.page_crossed = (base & 0xFF00) != (r.address & 0xFF00); break;
    case AM_REL: r.address = (uint16_t)(c->PC + 1 + (int8_t)bus_read(b, c->PC)); c->PC++; break;
    default: break;
    }
    return r;
}

static void interrupt(CPU6502 *c, Bus6502 *b, uint16_t vector, bool brk)
{
    push(c, b, (uint8_t)(c->PC >> 8)); push(c, b, (uint8_t)c->PC);
    push(c, b, (uint8_t)((c->P | FLAG_U | (brk ? FLAG_B : 0)) & (brk ? 0xFF : ~FLAG_B)));
    c->P |= FLAG_I; c->PC = bus_read16(b, vector);
}

static void compare(CPU6502 *c, uint8_t reg, uint8_t val)
{ uint8_t out = (uint8_t)(reg - val); set_flag(c, FLAG_C, reg >= val); set_nz(c, out); }

static void adc(CPU6502 *c, uint8_t m)
{
    uint8_t a = c->A, carry = !!(c->P & FLAG_C); uint16_t binary = (uint16_t)a + m + carry;
    if ((c->P & FLAG_D) && c->decimal_enabled) {
        uint16_t low=(a&0x0F)+(m&0x0F)+carry;
        uint16_t intermediate;
        if(low>=0x0A)low=((low+6)&0x0F)+0x10;
        intermediate=(a&0xF0)+(m&0xF0)+low;
        set_flag(c,FLAG_Z,(uint8_t)binary==0);
        set_flag(c,FLAG_N,intermediate&0x80);
        set_flag(c,FLAG_V,(~(a^m)&(a^(uint8_t)intermediate)&0x80)!=0);
        set_flag(c,FLAG_C,intermediate>=0xA0);
        if(intermediate>=0xA0)intermediate+=0x60;
        c->A=(uint8_t)intermediate;
    } else {
        set_flag(c,FLAG_V,(~(a^m)&(a^(uint8_t)binary)&0x80)!=0);
        set_flag(c, FLAG_C, binary > 0xFF); c->A = (uint8_t)binary; set_nz(c, c->A);
    }
}

static void sbc(CPU6502 *c, uint8_t m)
{
    uint8_t a = c->A, carry = !!(c->P & FLAG_C); uint16_t binary = (uint16_t)a + (uint8_t)~m + carry;
    uint8_t result = (uint8_t)binary;
    set_flag(c, FLAG_V, ((a ^ result) & (a ^ m) & 0x80) != 0);
    set_flag(c, FLAG_C, binary > 0xFF); set_nz(c, result);
    if ((c->P & FLAG_D) && c->decimal_enabled) {
        int16_t lo = (a & 15) - (m & 15) - (carry ? 0 : 1);
        int16_t hi = (a >> 4) - (m >> 4);
        if (lo < 0) { lo -= 6; hi--; }
        if (hi < 0) hi -= 6;
        c->A = (uint8_t)(((hi * 16) & 0xF0) | (lo & 0x0F));
    } else c->A = result;
}

static bool is_branch(operation_t op)
{ return op >= O_BCC && op <= O_BVS && op != O_BIT && op != O_BRK; }

cpu_step_result_t cpu_step(CPU6502 *c, Bus6502 *b)
{
    cpu_step_result_t out = {CPU_STEP_OK, 0, 0}; opcode_t d; addr_result_t ar; uint8_t v, old; bool take = false;
    if (c->nmi_pending) { c->nmi_pending = false; interrupt(c,b,0xFFFA,false); out.cycles=7; return out; }
    if (c->irq_asserted && !(c->P & FLAG_I)) { interrupt(c,b,0xFFFE,false); out.cycles=7; return out; }
    out.opcode = bus_read(b, c->PC++); d = ops[out.opcode];
    if (d.op == O_BAD) { out.status = CPU_STEP_ILLEGAL_OPCODE; return out; }
    if(d.op==O_JSR){uint8_t low=bus_read(b,c->PC++);uint16_t ret=c->PC;push(c,b,(uint8_t)(ret>>8));push(c,b,(uint8_t)ret);c->PC=(uint16_t)(low|((uint16_t)bus_read(b,c->PC)<<8));out.cycles=6;c->P|=FLAG_U;return out;}
    ar = address(c,b,d.mode); out.cycles = d.cycles + (d.page && ar.page_crossed);
    v = (d.mode == AM_IMP || d.mode == AM_ACC || d.mode == AM_REL) ? 0 : bus_read(b,ar.address);
    if (is_branch(d.op)) {
        switch(d.op) { case O_BCC:take=!(c->P&FLAG_C);break; case O_BCS:take=c->P&FLAG_C;break; case O_BEQ:take=c->P&FLAG_Z;break; case O_BMI:take=c->P&FLAG_N;break; case O_BNE:take=!(c->P&FLAG_Z);break; case O_BPL:take=!(c->P&FLAG_N);break; case O_BVC:take=!(c->P&FLAG_V);break; case O_BVS:take=c->P&FLAG_V;break; default:break; }
        if (take) { uint16_t from=c->PC; c->PC=ar.address; out.cycles += 1 + ((from&0xFF00)!=(c->PC&0xFF00)); } return out;
    }
    switch (d.op) {
    case O_ADC: adc(c,v); break; case O_AND:c->A&=v;set_nz(c,c->A);break;
    case O_ASL: old=d.mode==AM_ACC?c->A:v;set_flag(c,FLAG_C,old&0x80);old<<=1;set_nz(c,old);if(d.mode==AM_ACC)c->A=old;else bus_write(b,ar.address,old);break;
    case O_BIT:set_flag(c,FLAG_Z,(c->A&v)==0);set_flag(c,FLAG_N,v&0x80);set_flag(c,FLAG_V,v&0x40);break;
    case O_BRK:c->PC++;interrupt(c,b,0xFFFE,true);break;
    case O_CLC:c->P&=~FLAG_C;break; case O_CLD:c->P&=~FLAG_D;break; case O_CLI:c->P&=~FLAG_I;break; case O_CLV:c->P&=~FLAG_V;break;
    case O_CMP:compare(c,c->A,v);break; case O_CPX:compare(c,c->X,v);break; case O_CPY:compare(c,c->Y,v);break;
    case O_DEC:v--;bus_write(b,ar.address,v);set_nz(c,v);break; case O_DEX:c->X--;set_nz(c,c->X);break; case O_DEY:c->Y--;set_nz(c,c->Y);break;
    case O_EOR:c->A^=v;set_nz(c,c->A);break; case O_INC:v++;bus_write(b,ar.address,v);set_nz(c,v);break; case O_INX:c->X++;set_nz(c,c->X);break; case O_INY:c->Y++;set_nz(c,c->Y);break;
    case O_JMP:c->PC=ar.address;break; case O_JSR:break;
    case O_LDA:c->A=v;set_nz(c,v);break; case O_LDX:c->X=v;set_nz(c,v);break; case O_LDY:c->Y=v;set_nz(c,v);break;
    case O_LSR:old=d.mode==AM_ACC?c->A:v;set_flag(c,FLAG_C,old&1);old>>=1;set_nz(c,old);if(d.mode==AM_ACC)c->A=old;else bus_write(b,ar.address,old);break;
    case O_NOP:break; case O_ORA:c->A|=v;set_nz(c,c->A);break; case O_PHA:push(c,b,c->A);break; case O_PHP:push(c,b,c->P|FLAG_B|FLAG_U);break;
    case O_PLA:c->A=pull(c,b);set_nz(c,c->A);break; case O_PLP:c->P=(pull(c,b)&~FLAG_B)|FLAG_U;break;
    case O_ROL:old=d.mode==AM_ACC?c->A:v;{bool nc=old&0x80;old=(uint8_t)((old<<1)|!!(c->P&FLAG_C));set_flag(c,FLAG_C,nc);}set_nz(c,old);if(d.mode==AM_ACC)c->A=old;else bus_write(b,ar.address,old);break;
    case O_ROR:old=d.mode==AM_ACC?c->A:v;{bool nc=old&1;old=(uint8_t)((old>>1)|((c->P&FLAG_C)?0x80:0));set_flag(c,FLAG_C,nc);}set_nz(c,old);if(d.mode==AM_ACC)c->A=old;else bus_write(b,ar.address,old);break;
    case O_RTI:c->P=(pull(c,b)&~FLAG_B)|FLAG_U;c->PC=pull(c,b);c->PC|=(uint16_t)pull(c,b)<<8;break;
    case O_RTS:c->PC=pull(c,b);c->PC|=(uint16_t)pull(c,b)<<8;c->PC++;break; case O_SBC:sbc(c,v);break;
    case O_SEC:c->P|=FLAG_C;break; case O_SED:c->P|=FLAG_D;break; case O_SEI:c->P|=FLAG_I;break;
    case O_STA:bus_write(b,ar.address,c->A);break; case O_STX:bus_write(b,ar.address,c->X);break; case O_STY:bus_write(b,ar.address,c->Y);break;
    case O_TAX:c->X=c->A;set_nz(c,c->X);break; case O_TAY:c->Y=c->A;set_nz(c,c->Y);break; case O_TSX:c->X=c->SP;set_nz(c,c->X);break;
    case O_TXA:c->A=c->X;set_nz(c,c->A);break; case O_TXS:c->SP=c->X;break; case O_TYA:c->A=c->Y;set_nz(c,c->A);break;
    case O_LAX:c->A=c->X=v;set_nz(c,v);break;
    case O_SAX:bus_write(b,ar.address,c->A&c->X);break;
    case O_DCP:v--;bus_write(b,ar.address,v);compare(c,c->A,v);break;
    case O_ISC:v++;bus_write(b,ar.address,v);sbc(c,v);break;
    case O_SLO:set_flag(c,FLAG_C,v&0x80);v<<=1;bus_write(b,ar.address,v);c->A|=v;set_nz(c,c->A);break;
    case O_RLA:{bool nc=v&0x80;v=(uint8_t)((v<<1)|!!(c->P&FLAG_C));set_flag(c,FLAG_C,nc);bus_write(b,ar.address,v);c->A&=v;set_nz(c,c->A);}break;
    case O_SRE:set_flag(c,FLAG_C,v&1);v>>=1;bus_write(b,ar.address,v);c->A^=v;set_nz(c,c->A);break;
    case O_RRA:{bool nc=v&1;v=(uint8_t)((v>>1)|((c->P&FLAG_C)?0x80:0));set_flag(c,FLAG_C,nc);bus_write(b,ar.address,v);adc(c,v);}break;
    case O_ANC:c->A&=v;set_nz(c,c->A);set_flag(c,FLAG_C,c->A&0x80);break;
    case O_ALR:c->A&=v;set_flag(c,FLAG_C,c->A&1);c->A>>=1;set_nz(c,c->A);break;
    case O_ARR:{uint8_t anded=c->A&v,result=(uint8_t)((anded>>1)|((c->P&FLAG_C)?0x80:0));set_nz(c,result);set_flag(c,FLAG_V,((result>>6)^(result>>5))&1);if((c->P&FLAG_D)&&c->decimal_enabled){if((anded&0x0F)+(anded&1)>5)result=(uint8_t)((result&0xF0)|((result+6)&0x0F));if((anded&0xF0)+(anded&0x10)>0x50){result+=0x60;set_flag(c,FLAG_C,true);}else set_flag(c,FLAG_C,false);c->A=result;}else{c->A=result;set_flag(c,FLAG_C,result&0x40);}}break;
    case O_AXS:{uint8_t anded=c->A&c->X;set_flag(c,FLAG_C,anded>=v);c->X=(uint8_t)(anded-v);set_nz(c,c->X);}break;
    default:break;
    }
    c->P |= FLAG_U;
    return out;
}
