#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus.h"
#include "cpu.h"
#include "loader.h"
#include "opcode.h"
#include "state.h"

static unsigned long number(const char *s,int *ok)
{ char *end;unsigned long n=strtoul(s,&end,0);while(isspace((unsigned char)*end))end++;*ok=*s&&!*end&&n<=0xFFFF;return n; }
static void show(CPU6502 *c,Bus6502 *b)
{ char text[64];cpu_disassemble(b,c->PC,text,sizeof text);printf("PC:%04X A:%02X X:%02X Y:%02X P:%02X SP:%02X  %s\n",c->PC,c->A,c->X,c->Y,c->P,c->SP,text); }
static int one(CPU6502 *c,Bus6502 *b)
{ uint16_t pc=c->PC;cpu_step_result_t r=cpu_step(c,b);if(r.status!=CPU_STEP_OK){fprintf(stderr,"illegal opcode $%02X at $%04X\n",r.opcode,pc);return 0;}return 1; }

int main(int argc,char **argv)
{
    Bus6502 bus;CPU6502 cpu;size_t loaded;uint8_t breaks[65536]={0};char line[256];uint16_t load=0,start=0;
    if(argc<2||argc>4){fprintf(stderr,"usage: %s ROM [LOAD [START]]\n",argv[0]);return 2;}
    if(argc>2)load=(uint16_t)strtoul(argv[2],NULL,0);
    if(argc>3)start=(uint16_t)strtoul(argv[3],NULL,0);else start=load;
    bus_init(&bus);if(load_binary(&bus,argv[1],load,&loaded)){fprintf(stderr,"could not load ROM\n");return 1;}
    bus_write16(&bus,0xFFFC,start);cpu_reset(&cpu,&bus);printf("Loaded %zu bytes. Commands: s, c, b, r, m, w, save, load, q, h\n",loaded);show(&cpu,&bus);
    while(fputs("dbg> ",stdout),fflush(stdout),fgets(line,sizeof line,stdin)){
        char *cmd=strtok(line," \t\r\n"),*arg=strtok(NULL," \t\r\n"),*arg2=strtok(NULL," \t\r\n");int ok=0;
        if(!cmd)continue;
        if(!strcmp(cmd,"q")||!strcmp(cmd,"quit"))break;
        else if(!strcmp(cmd,"s")||!strcmp(cmd,"step")){unsigned long n=arg?strtoul(arg,NULL,0):1;while(n--&&one(&cpu,&bus));show(&cpu,&bus);}
        else if(!strcmp(cmd,"c")||!strcmp(cmd,"continue")){for(unsigned long n=0;n<100000000&&one(&cpu,&bus);n++)if(breaks[cpu.PC])break;show(&cpu,&bus);}
        else if(!strcmp(cmd,"b")||!strcmp(cmd,"break")){if(!arg){puts("break requires an address");continue;}unsigned long a=number(arg,&ok);if(ok){breaks[a]^=1;printf("breakpoint $%04lX %s\n",a,breaks[a]?"set":"cleared");}}
        else if(!strcmp(cmd,"r")||!strcmp(cmd,"registers"))show(&cpu,&bus);
        else if(!strcmp(cmd,"m")||!strcmp(cmd,"memory")){unsigned long a=arg?number(arg,&ok):cpu.PC,n=arg2?strtoul(arg2,NULL,0):64;if(!arg)ok=1;if(ok)for(unsigned long i=0;i<n;i++){if(!(i%16))printf("\n%04lX: ",(a+i)&0xFFFF);printf("%02X ",bus_read(&bus,(uint16_t)(a+i)));}putchar('\n');}
        else if(!strcmp(cmd,"w")||!strcmp(cmd,"write")){unsigned long a=arg?number(arg,&ok):0;int ok2=0;unsigned long v=arg2?number(arg2,&ok2):0;if(ok&&ok2&&v<=0xFF)bus_write(&bus,(uint16_t)a,(uint8_t)v);else puts("write ADDRESS BYTE");}
        else if(!strcmp(cmd,"save")){const char *p=arg?arg:"debug.state";printf("%s\n",state_save(p,&cpu,&bus)?"save failed":"state saved");}
        else if(!strcmp(cmd,"load")){const char *p=arg?arg:"debug.state";printf("%s\n",state_load(p,&cpu,&bus)?"load failed":"state loaded");show(&cpu,&bus);}
        else if(!strcmp(cmd,"d")||!strcmp(cmd,"disassemble")){unsigned long a=arg?number(arg,&ok):cpu.PC,n=arg2?strtoul(arg2,NULL,0):12;if(!arg)ok=1;if(ok)while(n--){char text[64];size_t bytes=cpu_disassemble(&bus,(uint16_t)a,text,sizeof text);printf("%04lX  %s\n",a,text);a=(a+bytes)&0xFFFF;}}
        else puts("s [N] | c | b ADDR | r | m [ADDR [N]] | w ADDR BYTE | d [ADDR [N]] | save/load [FILE] | q");
    }
    return 0;
}
