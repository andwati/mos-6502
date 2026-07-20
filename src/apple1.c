#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "apple1.h"

static bool apple1_read(void *userdata,uint16_t addr,uint8_t *value)
{
    Apple1IO *io=userdata;
    if(addr==0xD010){*value=io->key|0x80;io->key_ready=false;return true;}
    if(addr==0xD011){*value=io->key_ready?0x80:0;return true;}
    if(addr==0xD012||addr==0xD013){*value=0;return true;}
    return false;
}

static bool apple1_write(void *userdata,uint16_t addr,uint8_t value)
{
    Apple1IO *io=userdata;
    if(addr==0xD012){
        uint8_t c=value&0x7F;if(c=='\r')c='\n';
        if(io->output_length+1<sizeof io->output){io->output[io->output_length++]=(char)c;io->output[io->output_length]=0;}
        if(io->stdout_enabled){putchar(c);fflush(stdout);}return true;
    }
    if(addr==0xD010||addr==0xD011||addr==0xD013)return true;
    return false;
}

void apple1_init(Apple1IO *io,Bus6502 *bus)
{ memset(io,0,sizeof(*io));bus_set_hooks(bus,apple1_read,apple1_write,io); }
void apple1_key(Apple1IO *io,uint8_t ascii)
{ io->key=(uint8_t)toupper(ascii);io->key_ready=true; }
void apple1_enable_stdout(Apple1IO *io,bool enabled){io->stdout_enabled=enabled;}
