#include <stdio.h>
#include <stdlib.h>
#include "bus.h"
#include "cpu.h"
#include "loader.h"

static unsigned long number(const char *s)
{
    char *end; unsigned long n=strtoul(s,&end,0);
    if(!*s||*end||n>0xFFFF){fprintf(stderr,"invalid 16-bit number: %s\n",s);exit(2);}return n;
}

int main(int argc,char **argv)
{
    Bus6502 bus;CPU6502 cpu;size_t loaded;uint16_t load,start,success;unsigned long limit=100000000;
    if(argc<5||argc>6){fprintf(stderr,"usage: %s ROM LOAD START SUCCESS [LIMIT]\n",argv[0]);return 2;}
    load=(uint16_t)number(argv[2]);start=(uint16_t)number(argv[3]);success=(uint16_t)number(argv[4]);
    if(argc==6){char *end;limit=strtoul(argv[5],&end,0);if(!*argv[5]||*end||!limit){fprintf(stderr,"invalid limit\n");return 2;}}
    bus_init(&bus);if(load_binary(&bus,argv[1],load,&loaded)){fprintf(stderr,"failed to load %s\n",argv[1]);return 1;}
    bus_write16(&bus,0xFFFC,start);cpu_reset(&cpu,&bus);
    for(unsigned long i=0;i<limit;i++){
        uint16_t before=cpu.PC;cpu_step_result_t r=cpu_step(&cpu,&bus);
        if(r.status!=CPU_STEP_OK){fprintf(stderr,"illegal $%02X at $%04X after %lu instructions\n",r.opcode,before,i);return 1;}
        if(cpu.PC==success){printf("success at $%04X after %lu instructions (%zu-byte ROM)\n",success,i+1,loaded);return 0;}
    }
    fprintf(stderr,"limit reached at $%04X\n",cpu.PC);return 1;
}
