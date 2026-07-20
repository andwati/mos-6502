#include <stdio.h>
#include "bus.h"
#include "cpu.h"
#include "loader.h"

int main(int argc,char **argv)
{
    Bus6502 bus;CPU6502 cpu;size_t loaded;const unsigned long limit=200000000;
    if(argc!=2){fprintf(stderr,"usage: %s 6502_decimal_test.bin\n",argv[0]);return 2;}
    bus_init(&bus);
    if(load_binary(&bus,argv[1],0x0200,&loaded)){fprintf(stderr,"failed to load %s\n",argv[1]);return 1;}
    bus_write16(&bus,0xFFFC,0x0200);cpu_reset(&cpu,&bus);
    for(unsigned long i=0;i<limit;i++){
        uint16_t pc=cpu.PC;
        if(bus.memory.data[pc]==0xDB){
            if(bus.memory.data[0x000B]==0){printf("decimal success after %lu instructions (%zu-byte ROM)\n",i,loaded);return 0;}
            fprintf(stderr,"decimal failure at stop marker, ERROR=$%02X\n",bus.memory.data[0x000B]);return 1;
        }
        cpu_step_result_t r=cpu_step(&cpu,&bus);
        if(r.status==CPU_STEP_ILLEGAL_OPCODE){
            if(r.opcode==0xDB&&bus.memory.data[0x000B]==0){printf("decimal success after %lu instructions (%zu-byte ROM)\n",i,loaded);return 0;}
            fprintf(stderr,"decimal failure: opcode $%02X at $%04X, ERROR=$%02X N1=$%02X N2=$%02X "
                           "DA=$%02X flags=$%02X expectedA=$%02X N=$%02X V=$%02X Z=$%02X C=$%02X\n",
                    r.opcode,pc,bus.memory.data[0x000B],bus.memory.data[0],bus.memory.data[1],
                    bus.memory.data[4],bus.memory.data[5],bus.memory.data[6],bus.memory.data[7],
                    bus.memory.data[8],bus.memory.data[9],bus.memory.data[10]);return 1;
        }
    }
    fprintf(stderr,"decimal test instruction limit reached at $%04X\n",cpu.PC);return 1;
}
