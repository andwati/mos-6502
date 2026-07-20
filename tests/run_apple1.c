#include <stdio.h>
#include <string.h>
#include "apple1.h"
#include "cpu.h"
#include "loader.h"

int main(int argc,char **argv)
{
    static const char command[]="FF00.FF07\r";size_t sent=0,loaded;Bus6502 bus;CPU6502 cpu;Apple1IO io;
    if(argc!=2){fprintf(stderr,"usage: %s wozmon.bin\n",argv[0]);return 2;}
    bus_init(&bus);apple1_init(&io,&bus);
    if(load_binary(&bus,argv[1],0xFF00,&loaded)||loaded!=256)return 1;
    cpu_reset(&cpu,&bus);
    for(unsigned long i=0;i<2000000;i++){
        if(!io.key_ready&&sent<sizeof(command)-1)apple1_key(&io,(uint8_t)command[sent++]);
        if(cpu_step(&cpu,&bus).status!=CPU_STEP_OK){fprintf(stderr,"illegal instruction at $%04X\n",cpu.PC);return 1;}
        if(strstr(io.output,"FF00: D8 58 A0 7F 8C 12 D0 A9")){printf("Apple I WozMon boot/PIA test passed after %lu instructions\n",i+1);return 0;}
    }
    fprintf(stderr,"Apple I output mismatch:\n%s\n",io.output);return 1;
}
