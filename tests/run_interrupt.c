#include <stdio.h>
#include "bus.h"
#include "cpu.h"
#include "loader.h"
#include "opcode.h"

int main(int argc,char **argv)
{
    Bus6502 bus;CPU6502 cpu;size_t loaded;uint8_t previous=0;uint16_t history[32]={0};
    if(argc!=2){fprintf(stderr,"usage: %s 6502_interrupt_test.bin\n",argv[0]);return 2;}
    bus_init(&bus);
    if(load_binary(&bus,argv[1],0x000A,&loaded)){fprintf(stderr,"failed to load %s\n",argv[1]);return 1;}
    bus.memory.data[0xBFFC]=0;
    cpu.PC=0x0400;cpu.A=cpu.X=cpu.Y=0;cpu.SP=0xFD;cpu.P=FLAG_U|FLAG_I;
    cpu.nmi_pending=false;cpu.irq_asserted=false;
    for(unsigned long i=0;i<10000000;i++){
        uint16_t before=cpu.PC;history[i%32]=before;cpu_step_result_t r=cpu_step(&cpu,&bus);
        uint8_t feedback=bus.memory.data[0xBFFC]&3;
        cpu_set_irq(&cpu,(feedback&1)!=0);
        if((feedback&2)&&!(previous&2))cpu_request_nmi(&cpu);
        previous=feedback;
        if(r.status!=CPU_STEP_OK){fprintf(stderr,"illegal $%02X at $%04X\n",r.opcode,before);return 1;}
        if(cpu.PC==0x06F5){printf("interrupt success after %lu instructions (%zu-byte ROM)\n",i+1,loaded);return 0;}
        if(cpu.PC==before){
            fprintf(stderr,"interrupt trap at $%04X after %lu instructions\n",before,i+1);
            unsigned long first=i>31?i-31:0;
            for(unsigned long n=first;n<=i;n++){char text[48];uint16_t pc=history[n%32];cpu_disassemble(&bus,pc,text,sizeof text);fprintf(stderr,"  %04X  %s\n",pc,text);}return 1;
        }
    }
    fprintf(stderr,"interrupt test limit reached at $%04X\n",cpu.PC);return 1;
}
