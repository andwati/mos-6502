# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```sh
make          # build → bin/mos6502
make clean    # remove build/ and bin/
```

No test framework is wired up yet. Validation is planned via the **Klaus Dormann functional test suite** (see [docs/design.md](docs/design.md)): load `6502_functional_test.bin` at `$0000`, set PC to `$0400`, and confirm the emulator reaches the success loop rather than a trap.

## Architecture

This is a prototype MOS 6502 CPU emulator written in C. The design document at [docs/design.md](docs/design.md) is the authoritative reference — read it before making architectural changes.

### Module layout

| Module | Header | Source | Responsibility |
|---|---|---|---|
| CPU | [include/cpu.h](include/cpu.h) | [src/cpu.c](src/cpu.c) | `CPU6502` struct (A, X, Y, SP, P, PC), reset, state dump |
| Memory | [include/memory.h](include/memory.h) | [src/memory.c](src/memory.c) | Flat 64 KiB `uint8_t` array; `mem_read`/`mem_write`/`mem_reset` |
| Addressing | [include/addressing.h](include/addressing.h) | [src/addressing.c](src/addressing.c) | `addr_ctx_t` — callback-based bus abstraction (stub) |
| Opcode | [include/opcode.h](include/opcode.h) | [src/opcode.c](src/opcode.c) | Instruction decode/dispatch (stub) |
| Entry point | — | [src/main.c](src/main.c) | Instantiates `CPU6502` + `Memory`, calls reset, prints state |

### Key design decisions (from the design doc)

- **Memory access is always through `mem_read`/`mem_write`**, never direct array indexing from CPU logic. This keeps the door open for memory-mapped I/O and mirroring without touching the CPU.
- **`addr_ctx_t` uses function-pointer callbacks** (`read8`, `read16`) so the addressing layer is decoupled from the concrete `Memory` struct. Pass `userdata` to thread CPU + Memory through the callbacks.
- **Opcode dispatch will use a giant `switch` statement** (256 cases). The design doc explicitly rejects function-pointer tables and computed gotos in favour of compiler-optimised switch.
- **`PC` in `cpu.h` is currently typed `uint8_t`** — this is a known bug; it must be `uint16_t` to address the full 64 KiB space.
- **Status flags** are individual bit masks (`FLAG_C` through `FLAG_N`) applied to `cpu->P`.
- **Reset vector**: after `cpu_reset`, PC must be loaded from `$FFFC/$FFFD`; `cpu.c` currently leaves this for the caller.
- **Stack** lives at `$0100–$01FF`; SP is a page-relative offset, so the absolute address is `0x0100 | cpu->SP`.
- **Little-endian** 16-bit reads: LSB at lower address.

### Planned areas not yet implemented

- Addressing mode resolvers (`addressing.c` is empty)
- Opcode decode/execute loop (`opcode.c` is empty)
- Cycle counting
- Interrupt handling (NMI / IRQ / RESET vectors)
- BCD mode ADC/SBC
