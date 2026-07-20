#include <stdio.h>
#include <string.h>
#include "bus.h"
#include "apple1.h"
#include "cpu.h"
#include "state.h"
#include "nes.h"

static int tests, failures;
#define CHECK(expr) do { tests++; if (!(expr)) { failures++; fprintf(stderr,"%s:%d: check failed: %s\n",__FILE__,__LINE__,#expr); } } while(0)
#define EQ8(a,b) CHECK((uint8_t)(a)==(uint8_t)(b))
#define EQ16(a,b) CHECK((uint16_t)(a)==(uint16_t)(b))

static void setup(CPU6502 *cpu, Bus6502 *bus, uint16_t pc)
{ bus_init(bus); bus_write16(bus,0xFFFC,pc); cpu_reset(cpu,bus); }
static cpu_step_result_t step(CPU6502 *c, Bus6502 *b) { return cpu_step(c,b); }

static void test_reset_and_bus(void)
{
    CPU6502 c; Bus6502 b; setup(&c,&b,0xC123);
    EQ16(c.PC,0xC123); EQ8(c.SP,0xFD); EQ8(c.P,FLAG_U|FLAG_I);
    bus_write16(&b,0x1234,0xABCD); EQ16(bus_read16(&b,0x1234),0xABCD);
    b.easy6502_io=true; bus_set_keyboard(&b,'w'); EQ8(bus_read(&b,0xFF),'w');
    bus_seed_rng(&b,42); uint8_t x=bus_read(&b,0xFE); bus_seed_rng(&b,42); EQ8(bus_read(&b,0xFE),x);
}

static void test_load_store_and_modes(void)
{
    CPU6502 c; Bus6502 b; setup(&c,&b,0x8000);
    const uint8_t p[]={0xA2,0x02,0xA9,0x80,0x95,0xFF,0xA9,0x00,0xB5,0xFF};
    memcpy(&b.memory.data[0x8000],p,sizeof p);
    EQ8(step(&c,&b).cycles,2); EQ8(c.X,2); step(&c,&b); CHECK(c.P&FLAG_N);
    step(&c,&b); EQ8(bus_read(&b,1),0x80); step(&c,&b); step(&c,&b); EQ8(c.A,0x80);
    setup(&c,&b,0x80F0); c.X=1; b.memory.data[0x80F0]=0xBD; b.memory.data[0x80F1]=0xFF; b.memory.data[0x80F2]=0x12; b.memory.data[0x1300]=0x55;
    EQ8(step(&c,&b).cycles,5); EQ8(c.A,0x55);
}

static void test_indirect_and_control(void)
{
    CPU6502 c; Bus6502 b; setup(&c,&b,0x8000);
    b.memory.data[0x8000]=0x6C;b.memory.data[0x8001]=0xFF;b.memory.data[0x8002]=0x12;
    b.memory.data[0x12FF]=0x34;b.memory.data[0x1200]=0x12;b.memory.data[0x1300]=0x99;
    step(&c,&b); EQ16(c.PC,0x1234);
    setup(&c,&b,0x9000); b.memory.data[0x9000]=0x20;b.memory.data[0x9001]=0x00;b.memory.data[0x9002]=0xA0;b.memory.data[0xA000]=0x60;
    EQ8(step(&c,&b).cycles,6);EQ16(c.PC,0xA000);EQ8(c.SP,0xFB);step(&c,&b);EQ16(c.PC,0x9003);EQ8(c.SP,0xFD);
}

static void test_alu_and_flags(void)
{
    CPU6502 c; Bus6502 b; setup(&c,&b,0x8000);
    const uint8_t p[]={0xA9,0x7F,0x18,0x69,0x01,0x38,0xE9,0x01,0xC9,0x7F}; memcpy(&b.memory.data[0x8000],p,sizeof p);
    step(&c,&b);step(&c,&b);step(&c,&b);EQ8(c.A,0x80);CHECK(c.P&FLAG_V);CHECK(c.P&FLAG_N);
    step(&c,&b);step(&c,&b);EQ8(c.A,0x7F);CHECK(c.P&FLAG_C);step(&c,&b);CHECK(c.P&FLAG_Z);CHECK(c.P&FLAG_C);
    setup(&c,&b,0x8000); const uint8_t d[]={0xF8,0x18,0xA9,0x49,0x69,0x51};memcpy(&b.memory.data[0x8000],d,sizeof d);
    step(&c,&b);step(&c,&b);step(&c,&b);step(&c,&b);EQ8(c.A,0x00);CHECK(c.P&FLAG_C);
    setup(&c,&b,0x8000); const uint8_t s[]={0xF8,0x38,0xA9,0x00,0xE9,0x01};memcpy(&b.memory.data[0x8000],s,sizeof s);
    step(&c,&b);step(&c,&b);step(&c,&b);step(&c,&b);EQ8(c.A,0x99);CHECK(!(c.P&FLAG_C));
}

static void test_stack_interrupts(void)
{
    CPU6502 c; Bus6502 b; setup(&c,&b,0x8000); bus_write16(&b,0xFFFE,0x9000);b.memory.data[0x8000]=0x00;b.memory.data[0x9000]=0x40;
    c.P=FLAG_U|FLAG_C;step(&c,&b);EQ16(c.PC,0x9000);EQ8(c.SP,0xFA);CHECK(b.memory.data[0x1FB]&FLAG_B);step(&c,&b);EQ16(c.PC,0x8002);CHECK(c.P&FLAG_C);CHECK(!(c.P&FLAG_B));
    setup(&c,&b,0x8123);bus_write16(&b,0xFFFA,0xA000);cpu_request_nmi(&c);EQ8(step(&c,&b).cycles,7);EQ16(c.PC,0xA000);EQ8(b.memory.data[0x1FD],0x81);EQ8(b.memory.data[0x1FC],0x23);
}

static void test_branches_and_illegal(void)
{
    CPU6502 c; Bus6502 b; setup(&c,&b,0x80FD);c.P&=~FLAG_Z;b.memory.data[0x80FD]=0xD0;b.memory.data[0x80FE]=0x01;
    EQ8(step(&c,&b).cycles,4);EQ16(c.PC,0x8100);
    b.memory.data[0x8100]=0x02;cpu_step_result_t r=step(&c,&b);CHECK(r.status==CPU_STEP_ILLEGAL_OPCODE);EQ8(r.opcode,0x02);
}

static void test_stable_undocumented(void)
{
    CPU6502 c;Bus6502 b;setup(&c,&b,0x8000);
    const uint8_t p[]={0xA9,0xF0,0xA2,0xCC,0x87,0x10,0xA7,0x10,0xC7,0x10,
                       0xE7,0x10,0x0B,0x80,0x4B,0x0F,0xCB,0x01,0x1A};
    memcpy(&b.memory.data[0x8000],p,sizeof p);
    step(&c,&b);step(&c,&b);step(&c,&b);EQ8(b.memory.data[0x10],0xC0);
    step(&c,&b);EQ8(c.A,0xC0);EQ8(c.X,0xC0);
    step(&c,&b);EQ8(b.memory.data[0x10],0xBF);CHECK(c.P&FLAG_C);
    c.P|=FLAG_C;step(&c,&b);EQ8(b.memory.data[0x10],0xC0);EQ8(c.A,0x00);CHECK(c.P&FLAG_C);
    step(&c,&b);EQ8(c.A,0);CHECK(!(c.P&FLAG_C));
    c.A=0xFF;step(&c,&b);EQ8(c.A,7);CHECK(c.P&FLAG_C);
    c.A=0xFF;c.X=0x0F;step(&c,&b);EQ8(c.X,0x0E);CHECK(c.P&FLAG_C);
    EQ8(step(&c,&b).cycles,2);
}

static void test_portable_state(void)
{
    const char *path="/tmp/mos6502-test.state";CPU6502 c,restored;Bus6502 b,out;
    setup(&c,&b,0x8123);c.A=0x42;c.X=0x77;c.P=FLAG_U|FLAG_C;b.memory.data[0x2345]=0xAB;
    b.keyboard='d';b.easy6502_io=true;bus_seed_rng(&b,12345);
    CHECK(state_save(path,&c,&b)==0);bus_init(&out);memset(&restored,0,sizeof restored);
    CHECK(state_load(path,&restored,&out)==0);EQ16(restored.PC,0x8123);EQ8(restored.A,0x42);
    EQ8(restored.X,0x77);EQ8(restored.P,FLAG_U|FLAG_C);EQ8(out.memory.data[0x2345],0xAB);
    EQ8(out.keyboard,'d');CHECK(out.easy6502_io);CHECK(out.rng_state==12345);CHECK(out.framebuffer_dirty);
    CHECK(restored.decimal_enabled);
    remove(path);
}

static void test_2a03_ignores_decimal(void)
{
    CPU6502 c;Bus6502 b;setup(&c,&b,0x8000);c.decimal_enabled=false;
    const uint8_t p[]={0xF8,0x18,0xA9,0x49,0x69,0x51};memcpy(&b.memory.data[0x8000],p,sizeof p);
    step(&c,&b);step(&c,&b);step(&c,&b);step(&c,&b);EQ8(c.A,0x9A);CHECK(!(c.P&FLAG_C));
}

static void test_nes_nrom_bus(void)
{
    const char *path="/tmp/mos6502-test.nes";uint8_t header[16]={ 'N','E','S',0x1A,1,1 };
    uint8_t prg[16384]={0},chr[8192]={0};FILE *f=fopen(path,"wb");NES n;
    prg[0]=0xF8;prg[1]=0x18;prg[2]=0xA9;prg[3]=0x49;prg[4]=0x69;prg[5]=0x51;
    prg[0x3FFC]=0x00;prg[0x3FFD]=0x80;fwrite(header,1,sizeof header,f);fwrite(prg,1,sizeof prg,f);fwrite(chr,1,sizeof chr,f);fclose(f);
    CHECK(nes_load(&n,path)==0);nes_reset(&n);EQ16(n.cpu.PC,0x8000);CHECK(!n.cpu.decimal_enabled);
    bus_write(&n.bus,0x0001,0xAA);EQ8(bus_read(&n.bus,0x0801),0xAA);EQ8(bus_read(&n.bus,0xC000),0xF8);
    nes_step(&n);nes_step(&n);nes_step(&n);nes_step(&n);EQ8(n.cpu.A,0x9A);
    nes_set_controller(&n,0x05);bus_write(&n.bus,0x4016,1);bus_write(&n.bus,0x4016,0);
    EQ8(bus_read(&n.bus,0x4016)&1,1);EQ8(bus_read(&n.bus,0x4016)&1,0);EQ8(bus_read(&n.bus,0x4016)&1,1);
    remove(path);
}

static void write_test_rom(const char *path,uint8_t mapper,uint8_t prg_banks,uint8_t chr_banks)
{
    uint8_t h[16]={'N','E','S',0x1A,prg_banks,chr_banks,(uint8_t)(mapper<<4),0};FILE *f=fopen(path,"wb");
    fwrite(h,1,sizeof h,f);
    for(unsigned b=0;b<prg_banks;b++)for(unsigned i=0;i<16384;i++)fputc((int)b,f);
    for(unsigned b=0;b<chr_banks;b++)for(unsigned i=0;i<8192;i++)fputc((int)(0x40+b),f);
    fclose(f);
}

static void test_nes_mappers_and_apu(void)
{
    const char *u="/tmp/mos6502-urom.nes",*c="/tmp/mos6502-cnrom.nes";NES n;
    write_test_rom(u,2,4,0);CHECK(nes_load(&n,u)==0);EQ8(n.mapper,2);EQ8(bus_read(&n.bus,0x8000),0);EQ8(bus_read(&n.bus,0xC000),3);
    bus_write(&n.bus,0x8000,2);EQ8(bus_read(&n.bus,0x8000),2);EQ8(bus_read(&n.bus,0xC000),3);
    write_test_rom(c,3,2,4);CHECK(nes_load(&n,c)==0);EQ8(n.mapper,3);EQ8(n.chr[0],0x40);bus_write(&n.bus,0x8000,3);EQ8(n.chr_bank,3);
    bus_write(&n.bus,0x4000,0x3F);bus_write(&n.bus,0x4002,8);bus_write(&n.bus,0x4003,0);bus_write(&n.bus,0x4015,1);
    CHECK(bus_read(&n.bus,0x4015)==1);float peak=0;for(unsigned i=0;i<1000;i++){float s=nes_audio_sample(&n,48000);if(s>peak)peak=s;}CHECK(peak>0.1f);
    remove(u);remove(c);
}

static void test_nes_ppu_rendering(void)
{
    NES n;uint32_t pixels[256*240];memset(&n,0,sizeof n);n.chr_size=8192;n.chr_ram=true;n.ppu_mask=0x18;
    n.palette[0]=0;n.palette[1]=1;n.palette[0x11]=2;n.chr[0]=0x80;n.chr[16]=0x80;
    memset(n.oam,0xFF,sizeof n.oam);n.oam[0]=7;n.oam[1]=1;n.oam[2]=0;n.oam[3]=8;
    nes_render_frame(&n,pixels);CHECK(n.ppu_status&0x40);CHECK(!(n.ppu_status&0x20));CHECK(pixels[8*256+8]!=pixels[8*256+9]);
    for(unsigned s=0;s<9;s++){n.oam[s*4]=7;n.oam[s*4+3]=(uint8_t)(s*8+8);}nes_render_frame(&n,pixels);CHECK(n.ppu_status&0x20);
}

static void test_bus_hooks_and_apple1(void)
{
    Bus6502 b;Apple1IO io;bus_init(&b);apple1_init(&io,&b);apple1_key(&io,'a');
    EQ8(bus_read(&b,0xD011),0x80);EQ8(bus_read(&b,0xD010),'A'|0x80);EQ8(bus_read(&b,0xD011),0);
    bus_write(&b,0x1234,0x56);EQ8(bus_read(&b,0x1234),0x56);
}

int main(void)
{
    test_reset_and_bus();test_load_store_and_modes();test_indirect_and_control();
    test_alu_and_flags();test_stack_interrupts();test_branches_and_illegal();test_stable_undocumented();test_portable_state();test_bus_hooks_and_apple1();test_2a03_ignores_decimal();test_nes_nrom_bus();test_nes_mappers_and_apu();test_nes_ppu_rendering();
    printf("%d checks, %d failures\n",tests,failures);return failures?1:0;
}
