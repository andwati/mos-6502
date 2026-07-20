#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "state.h"

enum { HEADER_SIZE=25, STATE_SIZE=HEADER_SIZE+MEM_SIZE+4 };

static uint32_t crc32(const uint8_t *data,size_t size)
{
    uint32_t crc=0xFFFFFFFFu;
    for(size_t i=0;i<size;i++){crc^=data[i];for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^((uint32_t)-(int32_t)(crc&1)&0xEDB88320u);}
    return ~crc;
}

static void put32(uint8_t *p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}

int state_save(const char *path,const CPU6502 *c,const Bus6502 *b)
{
    uint8_t *data=calloc(1,STATE_SIZE);FILE *f;size_t wrote;
    if(!data)return ENOMEM;
    memcpy(data,"MOS6502",7);data[8]=2;data[9]=c->A;data[10]=c->X;data[11]=c->Y;
    data[12]=c->SP;data[13]=c->P;data[14]=(uint8_t)c->PC;data[15]=(uint8_t)(c->PC>>8);
    data[16]=c->nmi_pending;data[17]=c->irq_asserted;data[18]=c->decimal_enabled;
    data[19]=b->keyboard;data[20]=b->easy6502_io;put32(data+21,b->rng_state);
    memcpy(data+HEADER_SIZE,b->memory.data,MEM_SIZE);put32(data+HEADER_SIZE+MEM_SIZE,crc32(data,HEADER_SIZE+MEM_SIZE));
    f=fopen(path,"wb");if(!f){int e=errno;free(data);return e;}
    wrote=fwrite(data,1,STATE_SIZE,f);free(data);if(fclose(f)!=0||wrote!=STATE_SIZE)return EIO;return 0;
}

int state_load(const char *path,CPU6502 *c,Bus6502 *b)
{
    uint8_t *data=malloc(STATE_SIZE);FILE *f;size_t got;int extra;
    if(!data)return ENOMEM;
    f=fopen(path,"rb");
    if(!f){int e=errno;free(data);return e;}
    got=fread(data,1,STATE_SIZE,f);extra=fgetc(f);fclose(f);
    if(got!=STATE_SIZE||extra!=EOF||memcmp(data,"MOS6502",7)||data[8]!=2||
       get32(data+HEADER_SIZE+MEM_SIZE)!=crc32(data,HEADER_SIZE+MEM_SIZE)){free(data);return EINVAL;}
    c->A=data[9];c->X=data[10];c->Y=data[11];c->SP=data[12];c->P=data[13];
    c->PC=(uint16_t)data[14]|(uint16_t)data[15]<<8;c->nmi_pending=data[16]!=0;c->irq_asserted=data[17]!=0;c->decimal_enabled=data[18]!=0;
    b->keyboard=data[19];b->easy6502_io=data[20]!=0;b->rng_state=get32(data+21);
    memcpy(b->memory.data,data+HEADER_SIZE,MEM_SIZE);b->framebuffer_dirty=true;free(data);return 0;
}
