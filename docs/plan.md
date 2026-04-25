# MOS 6502 Emulator — Comprehensive Implementation Plan

## Context

The repository at `/home/ian/dev/mos-6502` is a prototype MOS 6502 emulator in C. The skeleton compiles and a basic `Memory` + `CPU6502` exists, but the fetch–decode–execute loop, addressing modes, and instruction set are absent. The authoritative architecture spec lives at [docs/design.md](../../dev/mos-6502/docs/design.md).

**Goal:** build a cycle-accurate NMOS 6502 emulator that passes the Klaus Dormann functional, decimal, and interrupt test suites, plus the `nestest` CPU log, and finally runs a real 6502 game (the canonical "6502 Snake" by Nick Morgan) end-to-end on a minimal memory-mapped framebuffer + keyboard.

The plan is organised into phases that ship and verify independently. A future Claude instance should execute one phase at a time and only proceed once that phase's verification step passes.

---

## Phase 0 — Already Implemented (status snapshot)

The repo skeleton compiles. What is in place:

- **Build system** ([Makefile](../../dev/mos-6502/Makefile)): `gcc -Wall -Wextra -O3 -Iinclude`, globs `src/*.c` → `build/*.o` → `bin/mos6502`. `make clean` wipes `build/` and `bin/`. New `.c` files under `src/` are picked up automatically.
- **Memory module** ([include/memory.h](../../dev/mos-6502/include/memory.h), [src/memory.c](../../dev/mos-6502/src/memory.c)): `Memory` is `uint8_t data[65536]`; `mem_read`, `mem_write`, `mem_reset` (memset zero) are direct array operations.
- **CPU module** ([include/cpu.h](../../dev/mos-6502/include/cpu.h), [src/cpu.c](../../dev/mos-6502/src/cpu.c)): `CPU6502` struct with A, X, Y, SP, PC, P; flag masks `FLAG_C/Z/I/D/B/U/V/N`; `cpu_reset` zeroes A/X/Y, sets `SP=0xFD`, `P=FLAG_U`, `PC=0x0000` (does **not** load reset vector); `cpu_print_state` dumps registers.
- **Addressing context stub** ([include/addressing.h](../../dev/mos-6502/include/addressing.h)): defines `addr_ctx_t` with `read8`, `read16`, `userdata` — a callback-based bus abstraction so addressing logic never touches `Memory` directly. `src/addressing.c` is empty.
- **Opcode module**: `src/opcode.c` is empty; `include/opcode.h` exists but is empty.
- **Entry point** ([src/main.c](../../dev/mos-6502/src/main.c)): `mem_reset` → `cpu_reset` → `cpu_print_state` → exit. No fetch–decode–execute loop.

### Bugs / discrepancies to carry into Phase 1

1. **`CPU6502.PC` is `uint8_t`** at [include/cpu.h:12](../../dev/mos-6502/include/cpu.h#L12). Must be `uint16_t`.
2. **`addr_ctx_t.read16` returns `void`** at [include/addressing.h:9](../../dev/mos-6502/include/addressing.h#L9). Must return `uint16_t`.
3. **SP reset value**: design doc says `0xFF`, code says `0xFD`. Real silicon converges on `0xFD` after the boot sequence — keep `0xFD`, treat the doc line as imprecise.
4. **Reset vector not loaded** — `cpu_reset` leaves `PC=0x0000` instead of fetching `$FFFC/$FFFD`.

---

## Phase 1 — Foundation Fixes

Correct types and reset semantics. No new behaviour.

**Files**: [include/cpu.h](../../dev/mos-6502/include/cpu.h), [include/addressing.h](../../dev/mos-6502/include/addressing.h), [src/cpu.c](../../dev/mos-6502/src/cpu.c), [src/main.c](../../dev/mos-6502/src/main.c).

**Changes**

- `uint8_t PC` → `uint16_t PC`.
- `read16` callback → `uint16_t (*read16)(uint16_t addr, void *userdata);`.
- `cpu_reset(CPU6502 *cpu, Memory *mem)` — after register init, `cpu->PC = mem_read(mem,0xFFFC) | (mem_read(mem,0xFFFD)<<8);`. `P = FLAG_U | FLAG_I` (I disabled at reset).

**Verify**: Write `0x34, 0x12` to `$FFFC/D` in `main.c`, run, confirm `PC: 0x1234`.

Reference: design.md §"Memory Map and Reserved Vectors", §"The Reset Sequence".

---

## Phase 2 — Memory Bus Abstraction

Every CPU access flows through `addr_ctx_t`. After this, swapping `Memory` for an MMIO bus is one line.

**Files**: [include/addressing.h](../../dev/mos-6502/include/addressing.h), [src/addressing.c](../../dev/mos-6502/src/addressing.c), new `src/bus.c`, [src/main.c](../../dev/mos-6502/src/main.c).

**Additions**

- `addr_ctx_t` gains `void (*write8)(uint16_t, uint8_t, void*)` and a `CPU6502 *cpu` field (resolvers need PC).
- `src/bus.c`: `bus_read8`, `bus_write8`, `bus_read16` — concrete `Memory*` callbacks.
- `addr_ctx_init(addr_ctx_t*, CPU6502*, Memory*)`.

**Verify**: `grep -n 'mem_read\|mem_write' src/cpu.c src/opcode.c` returns nothing (CPU never touches `Memory` directly). Reset-vector smoke test still passes through the ctx.

Reference: design.md §"Memory Access Implementation".

---

## Phase 3 — Test Harness Foundation (NEW)

Stand up an in-tree minimal test harness **before** writing the instruction set, so every later phase has assert-style verification. No external dependency — a header-only `assert(cond, msg)` is enough.

**Files**: new `tests/` directory, new `tests/test_framework.h`, new `tests/run_tests.c`, Makefile additions.

**Test framework**

- `tests/test_framework.h` provides:
  - `TEST(name)` — macro that registers a function pointer in a static table.
  - `EXPECT_EQ(actual, expected)`, `EXPECT_HEX8`, `EXPECT_HEX16`, `EXPECT_FLAG_SET(p,f)`, `EXPECT_FLAG_CLEAR(p,f)`.
  - On failure, print `file:line — got 0xAB, expected 0xCD` and continue, summing pass/fail at end.
- `tests/run_tests.c` is the entry point that calls every registered test. Each module gets a `tests/test_<module>.c` (cpu, memory, addressing, opcode_loads, opcode_alu, …).

**Makefile additions**

- `bin/run_tests` target: compiles `src/*.c` *minus* `main.c`, plus `tests/*.c`, with `-Itests`.
- `make test` → builds and runs `bin/run_tests`, exits non-zero if any test fails.
- `make` (default `all`) builds both `bin/mos6502` and `bin/run_tests`.

**Initial tests** (verify Phases 1–2)

- `test_memory.c`: round-trip `mem_write` → `mem_read`; `mem_reset` zeroes everything.
- `test_cpu_reset.c`: reset vector at `$FFFC/D` is honoured; SP=0xFD; flags = `U|I`.

**Verify**: `make test` passes both initial tests.

---

## Phase 4 — Addressing Mode Resolvers

One resolver per mode, each returning effective address + page-cross flag.

**Files**: [include/addressing.h](../../dev/mos-6502/include/addressing.h), [src/addressing.c](../../dev/mos-6502/src/addressing.c), `tests/test_addressing.c`.

**API**

```c
typedef struct { uint16_t addr; uint8_t page_crossed; } addr_result_t;

addr_result_t am_imm (addr_ctx_t *c);
addr_result_t am_zp  (addr_ctx_t *c);
addr_result_t am_zpx (addr_ctx_t *c);
addr_result_t am_zpy (addr_ctx_t *c);
addr_result_t am_abs (addr_ctx_t *c);
addr_result_t am_absx(addr_ctx_t *c);
addr_result_t am_absy(addr_ctx_t *c);
addr_result_t am_ind (addr_ctx_t *c);   // emulate $xxFF page-wrap bug
addr_result_t am_inx (addr_ctx_t *c);   // ZP wraps
addr_result_t am_iny (addr_ctx_t *c);   // ZP wraps; sets page_crossed
int8_t        am_rel (addr_ctx_t *c);
```

**NMOS quirks baked in now**

- `JMP ($xxFF)` reads high byte from `$xx00`, not `$(xxFF+1)`.
- ZP-wrap on ZPX/ZPY/INX/INY: pointer math stays in `$00–$FF`.

**Tests** (`tests/test_addressing.c`)

- Each mode: set up memory + registers, call resolver, assert `addr` and `page_crossed`.
- Edge cases: ZPX wrap (`zp=0xFF, X=0x02 → 0x01`); INY page-cross (`base=0x12FF, Y=0x01 → 0x1300, page_crossed=1`); JMP indirect at `$10FF`.

**Verify**: `make test` — addressing suite green.

Reference: design.md §"Addressing Mode Architecture".

---

## Phase 5 — Fetch-Decode-Execute Skeleton

Runnable loop, 256-case switch, `default:` traps.

**Files**: [include/opcode.h](../../dev/mos-6502/include/opcode.h), [src/opcode.c](../../dev/mos-6502/src/opcode.c), [src/main.c](../../dev/mos-6502/src/main.c).

**API**

```c
int  cpu_step(CPU6502 *cpu, addr_ctx_t *ctx);   // cycles used, -1 illegal
void cpu_run (CPU6502 *cpu, addr_ctx_t *ctx);   // until trap or self-loop
```

`main.c` switches from one-shot dump to `cpu_run`.

**Tests**: hand-assemble `LDA #$42; BRK` (`A9 42 00`) at `$0400`, expect `A=0x42` after one step (BRK is a sentinel until Phase 8).

Reference: design.md §"Opcode Decoding and Instruction Dispatch".

---

## Phase 6 — Instruction Set in Waves

Each wave is a separately verifiable sub-phase with its own `tests/test_opcode_<wave>.c`.

### 6a — Loads / Stores
`LDA, LDX, LDY, STA, STX, STY`. Loads update Z/N; stores do not.

### 6b — Register Transfers
`TAX, TAY, TXA, TYA, TSX, TXS`. TXS does **not** set flags.

### 6c — Logic
`AND, ORA, EOR`. `BIT`: bit 7→N, bit 6→V, Z from `A&M`.

### 6d — Shifts / Rotates
`ASL, LSR, ROL, ROR` — accumulator and memory variants. C from outgoing bit.

### 6e — Increments / Decrements
`INC, DEC, INX, INY, DEX, DEY`.

### 6f — Branches
`BPL, BMI, BVC, BVS, BCC, BCS, BNE, BEQ`. Reserve cycle accounting `2 + taken + page_crossed`.

### 6g — Jumps / Subroutines
`JMP abs`, `JMP indirect` (with page-wrap bug), `JSR`, `RTS`. JSR pushes `PC-1`; RTS pops and `PC++`. Stack at `0x0100 | SP`.

### 6h — Stack
`PHA, PLA, PHP, PLP`. PHP pushes `B|U` set; PLP ignores B, forces U. Static helpers `push8/pull8/push16/pull16`.

### 6i — Flag Set/Clear
`CLC, SEC, CLI, SEI, CLD, SED, CLV`.

### 6j — Compares
`CMP, CPX, CPY`. C set if `R >= M`; N/Z from low byte of `R-M`.

### 6k — ADC / SBC (binary only)
ADC: `s=A+M+C; C=s>0xFF; V=(~(A^M)&(A^s)&0x80)!=0; A=s&0xFF`. SBC = ADC with `M^0xFF`. Assert `!(P&FLAG_D)` until Phase 7.

### 6l — NOP
`0xEA`. All other unimplemented opcodes still trap.

**Per-wave verification**

- Each wave ships ≥ 6 micro-tests covering happy-path + flag edge cases + addressing-mode coverage.
- After 6k, `bin/mos6502 6502_functional_test.bin` runs far into the suite (will trap in BCD or interrupts — expected).

Reference: design.md §"Implementation of the Instruction Set", §"The Arithmetic Logic Unit", §"Control Flow, Branches, and the Stack".

---

## Phase 7 — BCD Mode (Bruce Clark)

ADC/SBC honour the D flag.

**Files**: [src/opcode.c](../../dev/mos-6502/src/opcode.c), `tests/test_bcd.c`.

Split into `adc_binary/adc_decimal/sbc_binary/sbc_decimal`; public ADC/SBC selects on `P&FLAG_D`.

**NMOS specifics**

- N, V, Z computed from the **binary intermediate**; only C reflects decimal carry-out.
- SBC decimal: nibble subtract with borrow → adjust by `-6` / `-0x60`.

**Tests**

- `49 + 51 = 00 C=1`; `12 - 03 = 09`; `99 + 01 = 00 C=1`; `00 - 01 = 99 C=0`.
- Smoke-run `6502_decimal_test.bin` (loader from Phase 11; for now wire a one-shot in `tests/`).

Reference: design.md §"Advanced Decimal Mode (BCD)".

---

## Phase 8 — Interrupts (NMI, IRQ, BRK, RTI)

**Files**: [include/cpu.h](../../dev/mos-6502/include/cpu.h) (add `nmi_pending`, `irq_line`), [src/opcode.c](../../dev/mos-6502/src/opcode.c).

**Sequence per design.md §"Interrupt Sequence Logic"**

1. Push PCH then PCL.
2. Push P (BRK pushes `B|U` set; hardware push has B clear, U set).
3. Set `FLAG_I`.
4. Load PC from vector (`$FFFA/B` NMI, `$FFFE/F` IRQ/BRK).
5. BRK skips one byte (push `PC+2`).

RTI pops P (force U set, B clear) then PCL then PCH; **no** `+1` (unlike RTS).

**Step-boundary priority**: NMI > IRQ (only if `!I`) > normal fetch.

**Tests**

- NMI: set `nmi_pending`, step once, assert push order on stack and PC reload.
- BRK + RTI round-trip restores PC and flags (modulo B/U canonicalisation).
- IRQ masked while `I=1`, fires when cleared.

Reference: design.md §"Interrupts and Hardware Interactions".

---

## Phase 9 — Cycle Counting

`cpu_step` returns accurate cycle counts.

**Files**: [src/opcode.c](../../dev/mos-6502/src/opcode.c).

- `static const uint8_t base_cycles[256]` (canonical table).
- `+1` if `page_crossed` for **read** ops on ABSX/ABSY/INY (LDA/LDX/LDY/EOR/AND/ORA/ADC/SBC/CMP/BIT). Stores never get the penalty.
- Branches: `+1` taken, `+1` more if page crossed.

**Tests** (`tests/test_cycles.c`)

- `LDA #$xx`=2; `LDA $xxxx`=4; `LDA $xxxx,X` page-crossed=5; `JSR`=6; taken+page-cross branch=4.

Reference: design.md §"Cycle-Level Timing Accuracy".

---

## Phase 10 — Undocumented (Illegal) Opcodes

Klaus Dormann's *functional* test does not require these, but `nestest` and many real games do. Implement the stable subset.

**Files**: [src/opcode.c](../../dev/mos-6502/src/opcode.c), `tests/test_undocumented.c`.

**Stable opcodes to support**

- `LAX` (LDA + LDX combined load)
- `SAX` (store `A & X`)
- `DCP` (DEC + CMP)
- `ISB` / `ISC` (INC + SBC)
- `SLO` (ASL + ORA)
- `RLA` (ROL + AND)
- `SRE` (LSR + EOR)
- `RRA` (ROR + ADC)
- `ANC`, `ALR`, `ARR`, `AXS` (immediate-mode oddballs)
- All unstable / "magic constant" opcodes (`SHA`, `SHX`, `SHY`, `TAS`, `LAS`, `XAA`) → log + treat as NOP for now (rarely used, behaviour varies by die).

**Verify**: dedicated unit tests + `nestest` log compare in Phase 11.

---

## Phase 11 — Validation Suites

Drive the emulator against published test ROMs. Each is a separate Make target so failures are bisectable.

**Files**: new `src/loader.c` + `include/loader.h`, `tests/run_suite.c` (CLI: `bin/run_suite <bin> <load_addr> <start_pc> [--success-pc <pc>]`), Makefile targets.

**Loader**

- `int load_binary(Memory*, const char *path, uint16_t load_addr)` — fread into `mem->data + load_addr`.

**Trap detection** (extends `cpu_run`)

- If the same opcode at the same PC executes twice in a row and is a self-branch (`BNE *`, `JMP *`), halt and report `success` if PC matches `--success-pc`, else `trap at 0xXXXX` with last 8 PCs as breadcrumbs.

**Suites integrated**

| Suite | Source | Load | Start PC | Success PC | Make target |
|---|---|---|---|---|---|
| `6502_functional_test.bin` | Klaus Dormann | `$0000` | `$0400` | `$3469` | `make test-functional` |
| `6502_decimal_test.bin` | Bruce Clark | `$0200` | `$0200` | self-loop on `ERROR=0` | `make test-decimal` |
| `6502_interrupt_test.bin` | Klaus Dormann | `$000A` | `$0400` | documented in suite | `make test-interrupts` |
| `nestest.nes` (CPU-only mode) | Kevitivity | `$C000` | `$C000` | log compare | `make test-nestest` |

**Interrupt-test MMIO shim**

- `bus_write8` checks for `addr == 0xBFFC`: bit 0 → `cpu->irq_line`, bit 1 → `cpu->nmi_pending`. This is what the Dormann interrupt test uses to assert IRQ/NMI lines.

**`nestest` log compare**

- A separate `tests/nestest_runner.c` that, after each step, formats the CPU state in the canonical `nestest.log` form (`PC  OP OPERANDS  A:.. X:.. Y:.. P:.. SP:.. CYC:..`) and diffs against `nestest.log` line-by-line. First mismatch fails the test.

**ROMs**: documented under `tests/roms/README.md` with attribution and download URLs (do **not** commit binaries unless licensing allows; provide a `make fetch-roms` target).

**Verify**: `make test-all` runs every suite; all pass before moving to Phase 12.

Reference: design.md §"Validation through the Klaus Dormann Test Suite".

---

## Phase 12 — Performance Benchmark

Quantify MIPS so regressions are visible.

**Files**: `tests/bench.c`, Makefile target `make bench`.

- Run a tight loop (e.g. `LDA #$01; CLC; ADC #$01; BNE -4`) for N seconds, count instructions, print MIPS.
- Compare to the design doc's expected range (150–500 MIPS on a 3 GHz host, design.md §"Performance Calculation").
- `make bench` writes a single-line CSV row to `bench.csv` (timestamp, git sha, MIPS) for tracking over time.

---

## Phase 13 — Memory-Mapped I/O Bus (Game-Ready)

Move from "Memory is the bus" to a real bus that supports MMIO. This is what makes a game runnable.

**Files**: rewrite `src/bus.c` to a region-table dispatch, new `include/bus.h`.

**Bus design**

```c
typedef struct {
    uint16_t start, end;
    uint8_t (*read )(uint16_t, void*);
    void    (*write)(uint16_t, uint8_t, void*);
    void    *userdata;
} bus_region_t;

typedef struct {
    bus_region_t regions[16];
    int n;
} bus_t;

void bus_map(bus_t*, uint16_t start, uint16_t end, ...);
uint8_t bus_read8 (uint16_t, void*);    // dispatches to region
void    bus_write8(uint16_t, uint8_t, void*);
```

**Default mapping**

- `$0000–$BFFF`: RAM (48 KiB).
- `$FE00–$FE00`: RNG read (returns `rand() & 0xFF`) — required by 6502 Snake.
- `$FF00–$FF00`: keyboard read (latest scancode, non-blocking).
- `$0200–$05FF`: framebuffer write-through to RAM, mirrored to a renderer.

**Verify**: rerun the test suites against the new bus — all still pass.

---

## Phase 14 — Display + Input Frontend (NEW)

Render the framebuffer and capture input. Two backends, picked at compile time.

**Files**: new `src/frontend_sdl.c`, `src/frontend_term.c`, Makefile flags `FRONTEND=sdl|term` (default `term`).

**Terminal frontend** (zero-dependency)

- 32×32 framebuffer at `$0200–$05FF`. Each byte = 4-bit colour index.
- After every `cpu_step` (or every N cycles), redraw via ANSI 256-colour blocks (`\x1b[48;5;<idx>m  `).
- Input via `termios` raw mode + non-blocking `read(STDIN)`; latest key written to `$FF`.

**SDL2 frontend** (optional)

- Same framebuffer, rendered via `SDL_CreateTexture` at scale 16x.
- `SDL_PollEvent` → keyboard scancode → `$FF`.
- Build with `make FRONTEND=sdl` (skip silently if `pkg-config --exists sdl2` fails).

**Verify**: `bin/mos6502 --frontend=term --rom=tests/roms/colorbars.bin` renders a static 4×4 colour grid by hand-assembling 16 `STA $0200,X; INX` writes.

---

## Phase 15 — Run a 6502 Game (capstone)

Run **6502 Snake** by Nick Morgan (the canonical Easy 6502 demo program). It uses exactly the MMIO map from Phase 13.

**Files**: `games/snake.asm` (source), `games/snake.bin` (assembled), `games/README.md` documenting controls.

**Flow**

1. Pull the public-domain `snake.asm` from Easy 6502 (https://skilldrick.github.io/easy6502/#snake) into `games/snake.asm`.
2. Assemble with `vasm6502_oldstyle` (or `cc65`'s `ca65`/`ld65`) → `games/snake.bin` (origin `$0600`).
3. Add a Make target `make snake`: builds emulator, assembles ROM, runs `bin/mos6502 --frontend=term games/snake.bin --start=0x0600`.
4. Reset vector convention: emulator writes `$0600` to `$FFFC/D` if `--start` is passed.

**Verify**: snake spawns, WASD moves it, eats apples, dies on self-collision.

**Stretch goals (optional follow-ups, not part of the capstone)**

- Port a second game: `games/tetris.bin` from a known 6502 implementation.
- Apple I emulation: map a 6850-style ACIA at `$D010–$D013`, load Wozmon at `$FF00`, get a working `\` prompt.
- Commodore PET BASIC ROM (requires sourcing a legally-redistributable image).

---

## Sequencing Summary

```
Phase 0 (done) → 1 → 2 → 3 (test harness) → 4 (modes) → 5 (loop)
              → 6a..6l (instruction waves, each ships green tests)
              → 7 (BCD) → 8 (interrupts) → 9 (cycles) → 10 (illegal ops)
              → 11 (validation suites — gate before moving on)
              → 12 (bench)
              → 13 (MMIO bus) → 14 (frontend) → 15 (snake game)
```

- Phases 1–5 strictly linear.
- Phase 6 waves are independent (order is suggested, not required).
- Phase 7 needs 6k; Phase 8 needs 6g+6h; Phase 9 can be folded in alongside 6 if convenient.
- **Hard gate**: Phase 11 must be fully green before starting Phase 13. Don't build a game on an emulator that fails Klaus Dormann.
- Phase 14 is independent of the bus rewrite as long as the framebuffer region exists.

---

## Critical Files

- [include/cpu.h](../../dev/mos-6502/include/cpu.h)
- [include/memory.h](../../dev/mos-6502/include/memory.h)
- [include/addressing.h](../../dev/mos-6502/include/addressing.h)
- [include/opcode.h](../../dev/mos-6502/include/opcode.h)
- [src/cpu.c](../../dev/mos-6502/src/cpu.c)
- [src/memory.c](../../dev/mos-6502/src/memory.c)
- [src/addressing.c](../../dev/mos-6502/src/addressing.c)
- [src/opcode.c](../../dev/mos-6502/src/opcode.c)
- [src/main.c](../../dev/mos-6502/src/main.c)
- [Makefile](../../dev/mos-6502/Makefile)
- New: `src/bus.c`, `src/loader.c`, `src/frontend_term.c`, `src/frontend_sdl.c`
- New: `tests/test_framework.h`, `tests/test_*.c`, `tests/run_tests.c`, `tests/run_suite.c`, `tests/bench.c`
- New: `games/snake.asm`, `games/snake.bin`

---

## End-to-End Verification

After Phase 15 the following must all succeed from a clean checkout:

```sh
make                              # builds bin/mos6502 and bin/run_tests
make test                         # all unit tests green
make fetch-roms                   # downloads validation ROMs into tests/roms/
make test-functional              # Klaus Dormann functional — success at $3469
make test-decimal                 # Bruce Clark BCD — error count = 0
make test-interrupts              # Klaus Dormann interrupt — success
make test-nestest                 # nestest CPU log diff — zero mismatches
make bench                        # prints MIPS, appends to bench.csv
make snake                        # opens snake game in terminal, playable
```

All green = emulator is functionally complete and validated against the standard 6502 test corpus, performant, and demonstrably running real software.
