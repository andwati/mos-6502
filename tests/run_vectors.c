#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bus.h"
#include "cpu.h"

static int byte(void){return fgetc(stdin);}
static int word(uint16_t *v){int a=byte(),b=byte();if(a==EOF||b==EOF)return 0;*v=(uint16_t)(a|(b<<8));return 1;}
static int regs(CPU6502 *c)
{
    int s,a,x,y,p;if(!word(&c->PC))return 0;s=byte();a=byte();x=byte();y=byte();p=byte();if(p==EOF)return 0;
    c->SP=(uint8_t)s;c->A=(uint8_t)a;c->X=(uint8_t)x;c->Y=(uint8_t)y;c->P=(uint8_t)p;c->decimal_enabled=true;return 1;
}
static int pairs(Bus6502 *b,uint16_t *addresses,uint8_t *values)
{
    uint16_t count;if(!word(&count))return -1;for(uint16_t i=0;i<count;i++){uint16_t a;int v;if(!word(&a)||(v=byte())==EOF)return -1;if(b)b->memory.data[a]=(uint8_t)v;if(addresses){addresses[i]=a;values[i]=(uint8_t)v;}}return count;
}
int main(void)
{
    unsigned long tests=0;int failures=0;
    for(;;){CPU6502 c,want;Bus6502 b;uint16_t addresses[64];uint8_t values[64];int count,cycles;
        bus_init(&b);memset(&c,0,sizeof c);memset(&want,0,sizeof want);if(!regs(&c))break;
        if(pairs(&b,NULL,NULL)<0||!regs(&want)||(count=pairs(NULL,addresses,values))<0||(cycles=byte())==EOF)return 2;
        cpu_step_result_t r=cpu_step(&c,&b);tests++;
        int bad=r.status!=CPU_STEP_OK||r.cycles!=(uint8_t)cycles||c.PC!=want.PC||c.SP!=want.SP||c.A!=want.A||c.X!=want.X||c.Y!=want.Y||c.P!=want.P;
        for(int i=0;i<count;i++)if(b.memory.data[addresses[i]]!=values[i])bad=1;
        if(bad&&failures++<20)fprintf(stderr,"vector %lu failed: got PC=%04X S=%02X A=%02X X=%02X Y=%02X P=%02X C=%u\n",tests,c.PC,c.SP,c.A,c.X,c.Y,c.P,r.cycles);
    }
    printf("%lu single-step vectors, %d failures\n",tests,failures);return failures?1:0;
}
