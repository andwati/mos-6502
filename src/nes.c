#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "nes.h"

static uint16_t nt_index(const NES *n,uint16_t a)
{ unsigned table=((a-0x2000)&0x0FFF)>>10,offset=a&0x3FF;if(n->mirroring==2)return (uint16_t)(table*0x400+offset);return (uint16_t)(((n->mirroring?table&1:table>>1)<<10)|offset); }
static uint8_t palette_index(uint16_t a)
{ uint8_t p=(uint8_t)(a&0x1F);if((p&0x13)==0x10)p-=0x10;return p; }
static size_t chr_offset(const NES *n,uint16_t a)
{ return n->mapper==3?((size_t)n->chr_bank*8192+(a&0x1FFF))%n->chr_size:(a&0x1FFF)%n->chr_size; }
static uint8_t ppu_read(NES *n,uint16_t a)
{ a&=0x3FFF;if(a<0x2000)return n->chr[chr_offset(n,a)];if(a<0x3F00)return n->nametable[nt_index(n,a)];return n->palette[palette_index(a)]; }
static void ppu_write(NES *n,uint16_t a,uint8_t v)
{ a&=0x3FFF;if(a<0x2000){if(n->chr_ram)n->chr[chr_offset(n,a)]=v;}else if(a<0x3F00)n->nametable[nt_index(n,a)]=v;else n->palette[palette_index(a)]=v; }

static size_t prg_offset(const NES *n,uint16_t a)
{
    if(n->mapper==2){size_t banks=n->prg_size/16384,bank=a<0xC000?n->prg_bank%(banks-1):banks-1;return bank*16384+(a&0x3FFF);}
    return (a-0x8000)%n->prg_size;
}

static bool nes_read(void *ud,uint16_t a,uint8_t *v)
{
    NES *n=ud;
    if(a<0x2000){*v=n->ram[a&0x7FF];return true;}
    if(a<0x4000){
        switch(a&7){
        case 2:*v=n->ppu_status;n->ppu_status&=0x7F;n->ppu_latch=0;return true;
        case 4:*v=n->oam[n->oam_addr];return true;
        case 7:{uint8_t value=ppu_read(n,n->ppu_addr);if((n->ppu_addr&0x3FFF)<0x3F00){*v=n->read_buffer;n->read_buffer=value;}else{*v=value;n->read_buffer=ppu_read(n,n->ppu_addr-0x1000);}n->ppu_addr+=(n->ppu_ctrl&4)?32:1;return true;}
        default:*v=0;return true;}
    }
    if(a==0x4016){*v=n->controller_shift&1;if(!n->controller_strobe)n->controller_shift=(n->controller_shift>>1)|0x80;return true;}
    if(a==0x4015){*v=n->apu_status;return true;}
    if(a>=0x4000&&a<0x4020){*v=0;return true;}
    if(a>=0x6000&&a<0x8000){*v=n->prg_ram[a-0x6000];return true;}
    if(a>=0x8000){*v=n->prg[prg_offset(n,a)];return true;}
    *v=0;return true;
}

static bool nes_write(void *ud,uint16_t a,uint8_t v)
{
    NES *n=ud;
    if(a<0x2000){n->ram[a&0x7FF]=v;return true;}
    if(a<0x4000){switch(a&7){case 0:n->ppu_ctrl=v;break;case 1:n->ppu_mask=v;break;case 3:n->oam_addr=v;break;case 4:n->oam[n->oam_addr++]=v;break;case 5:if(!n->ppu_latch)n->scroll_x=v;else n->scroll_y=v;n->ppu_latch^=1;break;case 6:if(!n->ppu_latch)n->ppu_addr=(uint16_t)(v&0x3F)<<8;else n->ppu_addr=(n->ppu_addr&0xFF00)|v;n->ppu_latch^=1;break;case 7:ppu_write(n,n->ppu_addr,v);n->ppu_addr+=(n->ppu_ctrl&4)?32:1;break;}return true;}
    if(a==0x4014){for(unsigned i=0;i<256;i++){uint8_t x;nes_read(n,(uint16_t)(v*256+i),&x);n->oam[n->oam_addr++]=x;}return true;}
    if(a==0x4016){n->controller_strobe=v&1;if(n->controller_strobe)n->controller_shift=n->controller;return true;}
    if(a>=0x4000&&a<0x4018){n->apu_reg[a-0x4000]=v;if(a==0x4015)n->apu_status=v&0x1F;return true;}
    if(a>=0x6000&&a<0x8000){n->prg_ram[a-0x6000]=v;return true;}
    if(a>=0x8000){if(n->mapper==2)n->prg_bank=v;if(n->mapper==3)n->chr_bank=v;return true;}return false;
}

int nes_load(NES *n,const char *path)
{
    uint8_t h[16];FILE *f=fopen(path,"rb");size_t prg,chr;unsigned mapper;
    if(!f)return errno;
    memset(n,0,sizeof(*n));
    if(fread(h,1,16,f)!=16||memcmp(h,"NES\x1A",4)){fclose(f);return EINVAL;}
    mapper=(h[6]>>4)|(h[7]&0xF0);if((mapper!=0&&mapper!=2&&mapper!=3)||h[4]<1){fclose(f);return ENOTSUP;}
    if(h[6]&4)fseek(f,512,SEEK_CUR);
    prg=(size_t)h[4]*16384;chr=(size_t)h[5]*8192;if(prg>sizeof n->prg||chr>sizeof n->chr){fclose(f);return EFBIG;}
    if(fread(n->prg,1,prg,f)!=prg||(chr&&fread(n->chr,1,chr,f)!=chr)){fclose(f);return EIO;}
    if(fgetc(f)!=EOF){fclose(f);return EFBIG;}fclose(f);
    n->prg_size=prg;n->chr_size=chr?chr:8192;n->chr_ram=chr==0;n->mapper=(uint8_t)mapper;n->mirroring=(h[6]&8)?2:(h[6]&1);
    bus_init(&n->bus);bus_set_hooks(&n->bus,nes_read,nes_write,n);return 0;
}

void nes_reset(NES *n){cpu_reset(&n->cpu,&n->bus);n->cpu.decimal_enabled=false;n->ppu_scanline=n->ppu_dot=0;n->frame_ready=false;n->noise_lfsr=1;}
cpu_step_result_t nes_step(NES *n)
{
    cpu_step_result_t r=cpu_step(&n->cpu,&n->bus);unsigned ticks=r.cycles*3;n->apu_cycles+=r.cycles;
    while(ticks--){if(++n->ppu_dot>=341){n->ppu_dot=0;if(++n->ppu_scanline>=262){n->ppu_scanline=0;n->frame_ready=true;}}
        if(n->ppu_scanline==241&&n->ppu_dot==1){n->ppu_status|=0x80;if(n->ppu_ctrl&0x80)cpu_request_nmi(&n->cpu);}
        if(n->ppu_scanline==261&&n->ppu_dot==1)n->ppu_status&=0x1F;}
    return r;
}
void nes_set_controller(NES *n,uint8_t buttons){n->controller=buttons;if(n->controller_strobe)n->controller_shift=buttons;}

float nes_audio_sample(NES *n,double rate)
{
    static const double cpu_hz=1789773.0;static const uint16_t noise_period[16]={4,8,16,32,64,96,128,160,202,254,380,508,762,1016,2034,4068};
    static const uint8_t duty_bits[4]={1,3,15,63};double mix=0.0;
    for(unsigned ch=0;ch<2;ch++)if(n->apu_status&(1u<<ch)){uint8_t *r=&n->apu_reg[ch*4];unsigned timer=r[2]|((r[3]&7)<<8);double freq=cpu_hz/(16.0*(timer+1));n->pulse_phase[ch]+=freq/rate;n->pulse_phase[ch]-=(unsigned)n->pulse_phase[ch];unsigned phase=(unsigned)(n->pulse_phase[ch]*8)&7;mix+=((duty_bits[r[0]>>6]>>phase)&1)?(r[0]&15)/30.0:0.0;}
    if(n->apu_status&4){uint8_t *r=&n->apu_reg[8];unsigned timer=n->apu_reg[10]|((n->apu_reg[11]&7)<<8);double freq=cpu_hz/(32.0*(timer+1));n->triangle_phase+=freq/rate;n->triangle_phase-=(unsigned)n->triangle_phase;double tri=n->triangle_phase<0.5?n->triangle_phase*4-1:3-n->triangle_phase*4;mix+=tri*(r[0]&0x7F?0.22:0.0);}
    if(n->apu_status&8){uint8_t *r=&n->apu_reg[12];double freq=cpu_hz/noise_period[n->apu_reg[14]&15];n->noise_phase+=freq/rate;if(n->noise_phase>=1){unsigned tap=(n->apu_reg[14]&0x80)?6:1;n->noise_lfsr=(uint16_t)((n->noise_lfsr>>1)|(((n->noise_lfsr^(n->noise_lfsr>>tap))&1)<<14));n->noise_phase-=1;}if(!(n->noise_lfsr&1))mix+=(r[0]&15)/45.0;}
    if(mix>1)mix=1;
    if(mix< -1)mix=-1;
    return (float)mix;
}

void nes_render_frame(NES *n,uint32_t px[256*240])
{
    static const uint32_t colors[64]={
      0x666666FF,0x002A88FF,0x1412A7FF,0x3B00A4FF,0x5C007EFF,0x6E0040FF,0x6C0600FF,0x561D00FF,0x333500FF,0x0B4800FF,0x005200FF,0x004F08FF,0x00404DFF,0,0,0,
      0xADADADFF,0x155FD9FF,0x4240FFFF,0x7527FEFF,0xA01ACCFF,0xB71E7BFF,0xB53120FF,0x994E00FF,0x6B6D00FF,0x388700FF,0x0C9300FF,0x008F32FF,0x007C8DFF,0,0,0,
      0xFFFFFFFF,0x64B0FFFF,0x9290FFFF,0xC676FFFF,0xF36AFFFF,0xFE6ECCFF,0xFE8170FF,0xEA9E22FF,0xBCBE00FF,0x88D800FF,0x5CE430FF,0x45E082FF,0x48CDDEFF,0x4F4F4FFF,0,0,
      0xFFFFFFFF,0xC0DFFFFF,0xD3D2FFFF,0xE8C8FFFF,0xFBC2FFFF,0xFEC4EAFF,0xFECCC5FF,0xF7D8A5FF,0xE4E594FF,0xCFEF96FF,0xBDF4ABFF,0xB3F3CCFF,0xB5EBF2FF,0xB8B8B8FF,0,0};
    uint8_t opaque[256*240]={0};uint16_t bg_pattern=(n->ppu_ctrl&0x10)?0x1000:0;n->ppu_status&=0x1F;
    for(unsigned y=0;y<240;y++)for(unsigned x=0;x<256;x++){
        unsigned wx=x+n->scroll_x+((n->ppu_ctrl&1)?256:0),wy=y+n->scroll_y+((n->ppu_ctrl&2)?240:0);
        unsigned tx=(wx/8)%64,ty=(wy/8)%60,nt=(tx/32)+(ty/30)*2,lx=tx%32,ly=ty%30;
        uint16_t base=(uint16_t)(0x2000+nt*0x400);uint8_t tile=ppu_read(n,(uint16_t)(base+ly*32+lx));
        uint8_t attr=ppu_read(n,(uint16_t)(base+0x3C0+(ly/4)*8+lx/4));unsigned shift=((ly&2)<<1)|(lx&2);
        uint16_t p=bg_pattern+tile*16+(wy&7);uint8_t bit=(uint8_t)(7-(wx&7));uint8_t index=(uint8_t)(((ppu_read(n,p)>>bit)&1)|(((ppu_read(n,p+8)>>bit)&1)<<1));
        if(!(n->ppu_mask&8)||(x<8&&!(n->ppu_mask&2)))index=0;
        unsigned pal=((attr>>shift)&3)*4+index;opaque[y*256+x]=index;px[y*256+x]=colors[n->palette[index?pal:0]&0x3F];
    }
    if(n->ppu_mask&0x10){unsigned height=(n->ppu_ctrl&0x20)?16:8;
    for(unsigned y=0;y<240;y++){unsigned count=0;for(unsigned s=0;s<64;s++){int sy=n->oam[s*4]+1;if((int)y>=sy&&(int)y<sy+(int)height&&++count>8)n->ppu_status|=0x20;}}
    for(int s=63;s>=0;s--){int sy=n->oam[s*4]+1,sx=n->oam[s*4+3];uint8_t tile=n->oam[s*4+1],attr=n->oam[s*4+2];
        for(unsigned py=0;py<height;py++)for(int pxl=0;pxl<8;pxl++){int yy=sy+(int)py,xx=sx+pxl;if(xx<0||xx>=256||yy<0||yy>=240||(xx<8&&!(n->ppu_mask&4)))continue;unsigned ry=(attr&0x80)?height-1-py:py,rx=(attr&0x40)?(unsigned)pxl:7-(unsigned)pxl;uint16_t base;
            if(height==16){base=(uint16_t)((tile&1)*0x1000+(tile&0xFE)*16+(ry>=8?16:0)+(ry&7));}else base=(uint16_t)(((n->ppu_ctrl&8)?0x1000:0)+tile*16+ry);
            uint8_t index=(uint8_t)(((ppu_read(n,base)>>rx)&1)|(((ppu_read(n,base+8)>>rx)&1)<<1));if(index&&s==0&&opaque[yy*256+xx]&&xx<255)n->ppu_status|=0x40;if(index&&(!(attr&0x20)||!opaque[yy*256+xx]))px[yy*256+xx]=colors[n->palette[0x10+(attr&3)*4+index]&0x3F];}}
    }
}
