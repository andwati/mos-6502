#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes.h"

static unsigned field(const char *line,const char *name)
{ const char *p=strstr(line,name);return p?(unsigned)strtoul(p+strlen(name),NULL,16):0xFFFFFFFFu; }

int main(int argc,char **argv)
{
    NES nes;FILE *log;char line[256];unsigned long lines=0,cycles=7;
    if(argc!=3){fprintf(stderr,"usage: %s nestest.nes nestest.log\n",argv[0]);return 2;}
    if(nes_load(&nes,argv[1])){fprintf(stderr,"failed to load nestest ROM\n");return 1;}
    log=fopen(argv[2],"r");if(!log){perror(argv[2]);return 1;}
    nes_reset(&nes);nes.cpu.PC=0xC000;nes.cpu.A=nes.cpu.X=nes.cpu.Y=0;nes.cpu.P=FLAG_U|FLAG_I;nes.cpu.SP=0xFD;
    while(fgets(line,sizeof line,log)){
        unsigned pc=(unsigned)strtoul(line,NULL,16),a=field(line,"A:"),x=field(line,"X:"),y=field(line,"Y:"),p=field(line,"P:"),sp=field(line,"SP:");
        const char *cy=strstr(line,"CYC:");unsigned long expected_cycles=cy?strtoul(cy+4,NULL,10):cycles;
        if(nes.cpu.PC!=pc||nes.cpu.A!=a||nes.cpu.X!=x||nes.cpu.Y!=y||nes.cpu.P!=p||nes.cpu.SP!=sp||cycles!=expected_cycles){
            fprintf(stderr,"nestest mismatch line %lu\nexpected: %sactual:   %04X A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n",lines+1,line,nes.cpu.PC,nes.cpu.A,nes.cpu.X,nes.cpu.Y,nes.cpu.P,nes.cpu.SP,cycles);fclose(log);return 1;
        }
        cpu_step_result_t r=nes_step(&nes);if(r.status!=CPU_STEP_OK){fprintf(stderr,"illegal opcode $%02X on line %lu\n",r.opcode,lines+1);fclose(log);return 1;}
        cycles+=r.cycles;lines++;
    }
    fclose(log);printf("nestest matched %lu instructions and cycle counts\n",lines);return lines==8991?0:1;
}
