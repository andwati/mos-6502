# Contributor notes

## Commands

```sh
make              # build bin/mos6502
make test         # core checks
make debug        # strict warning build and checks
make sanitize     # ASan/UBSan build and checks
make snake        # assemble and run the SDL game (requires SDL2 + cc65)
```

## Architecture

- `CPU6502` contains CPU-visible and interrupt-line state.
- `Bus6502` owns RAM and Easy6502 MMIO; CPU code only uses bus accessors.
- `cpu_step` executes one instruction or interrupt and returns status/opcode/cycles.
- `src/opcode.c` has a 256-entry descriptor table for all 151 official encodings.
- `src/state.c` owns the portable, versioned state file format.
- `src/apple1.c` maps Apple I keyboard/display PIA registers through bus hooks.
- `src/frontend.c` is an optional SDL2 frontend with a dependency-free stub.
- `tests/run_tests.c` is the in-tree test harness; `tests/run_suite.c` runs external ROMs.

The target is the NMOS 6502. Preserve decimal-mode flag behavior, zero-page
wrapping, the indirect-JMP page-wrap bug, stack push order, and page-cross cycle
rules. Do not silently accept undocumented opcodes.

The virtual machine exposes game state at `$00FB`, high/game score at `$00FC/$00FD`, random input
at `$00FE`, keyboard input at `$00FF`, and a 32x32 framebuffer at
`$0200-$05FF`.
