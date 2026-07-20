#include <stdio.h>
#include <string.h>
#include "bus.h"
#include "cpu.h"

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

int main(void)
{
    test_reset_and_bus();test_load_store_and_modes();test_indirect_and_control();
    test_alu_and_flags();test_stack_interrupts();test_branches_and_illegal();
    printf("%d checks, %d failures\n",tests,failures);return failures?1:0;
}
