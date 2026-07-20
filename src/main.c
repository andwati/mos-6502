#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bus.h"
#include "cpu.h"
#include "frontend.h"
#include "loader.h"
#include "opcode.h"

static unsigned long parse_number(const char *s, const char *name, unsigned long max)
{
    char *end; unsigned long value = strtoul(s, &end, 0);
    if (!*s || *end || value > max) {
        fprintf(stderr, "invalid %s: %s\n", name, s); exit(2);
    }
    return value;
}

static void usage(const char *program)
{
    fprintf(stderr, "usage: %s ROM [--load ADDRESS] [--start ADDRESS] "
                    "[--hz HZ] [--scale N] [--seed N] [--headless] [--trace]\n", program);
}

int main(int argc, char **argv)
{
    Bus6502 bus; CPU6502 cpu; const char *rom = NULL;
    uint16_t load = 0x0600, start = 0x0600; unsigned hz = 1000000, scale = 16;
    uint32_t seed = 0x6502; bool headless = false, trace=false; size_t loaded; int rc;
    for (int i=1;i<argc;i++) {
        if (!strcmp(argv[i],"--load") && i+1<argc) load=(uint16_t)parse_number(argv[++i],"load address",0xFFFF);
        else if (!strcmp(argv[i],"--start") && i+1<argc) start=(uint16_t)parse_number(argv[++i],"start address",0xFFFF);
        else if (!strcmp(argv[i],"--hz") && i+1<argc) hz=(unsigned)parse_number(argv[++i],"frequency",100000000);
        else if (!strcmp(argv[i],"--scale") && i+1<argc) scale=(unsigned)parse_number(argv[++i],"scale",128);
        else if (!strcmp(argv[i],"--seed") && i+1<argc) seed=(uint32_t)strtoul(argv[++i],NULL,0);
        else if (!strcmp(argv[i],"--headless")) headless=true;
        else if (!strcmp(argv[i],"--trace")) trace=true;
        else if (argv[i][0]=='-' || rom) { usage(argv[0]); return 2; }
        else rom=argv[i];
    }
    if (!rom || !hz || !scale) { usage(argv[0]); return 2; }
    bus_init(&bus); bus.easy6502_io=true; bus_seed_rng(&bus,seed);
    rc=load_binary(&bus,rom,load,&loaded);
    if (rc) { fprintf(stderr,"cannot load %s: %s\n",rom,strerror(rc)); return 1; }
    bus_write16(&bus,0xFFFC,start); cpu_reset(&cpu,&bus);
    printf("loaded %zu bytes at $%04X, starting at $%04X\n",loaded,load,start);
    if (!headless) return frontend_run(&cpu,&bus,hz,scale);
    for (unsigned long n=0;n<100000000ul;n++) {
        if(trace){char instruction[48];cpu_disassemble(&bus,cpu.PC,instruction,sizeof instruction);printf("%04X  %-18s A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",cpu.PC,instruction,cpu.A,cpu.X,cpu.Y,cpu.P,cpu.SP);}
        cpu_step_result_t step=cpu_step(&cpu,&bus);
        if (step.status != CPU_STEP_OK) { fprintf(stderr,"illegal opcode $%02X at $%04X\n",step.opcode,(uint16_t)(cpu.PC-1));return 1; }
    }
    fprintf(stderr,"instruction limit reached at $%04X\n",cpu.PC); return 1;
}
