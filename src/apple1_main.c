#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include "apple1.h"
#include "cpu.h"
#include "loader.h"

int main(int argc,char **argv)
{
    Bus6502 bus;CPU6502 cpu;Apple1IO io;size_t loaded;struct termios old,raw;int tty=0;
    if(argc!=2){fprintf(stderr,"usage: %s WOZMON.bin\n",argv[0]);return 2;}
    bus_init(&bus);apple1_init(&io,&bus);apple1_enable_stdout(&io,true);
    if(load_binary(&bus,argv[1],0xFF00,&loaded)||loaded!=256){fprintf(stderr,"Apple I monitor ROM must be exactly 256 bytes\n");return 1;}
    cpu_reset(&cpu,&bus);
    if(tcgetattr(STDIN_FILENO,&old)==0){tty=1;raw=old;cfmakeraw(&raw);tcsetattr(STDIN_FILENO,TCSANOW,&raw);}
    fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL)|O_NONBLOCK);
    for(;;){
        unsigned char key;if(read(STDIN_FILENO,&key,1)==1){if(key==3)break;apple1_key(&io,key);}
        for(unsigned n=0;n<1000;n++)if(cpu_step(&cpu,&bus).status!=CPU_STEP_OK)goto done;
    }
done:if(tty)tcsetattr(STDIN_FILENO,TCSANOW,&old);putchar('\n');return 0;
}
