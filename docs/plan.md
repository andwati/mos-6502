# Project status

## Implemented

- 64 KiB bus, reset vectors, deterministic RNG and Easy6502 keyboard/framebuffer MMIO.
- All official NMOS 6502 instructions and opcode encodings.
- Binary and NMOS decimal ADC/SBC, status flags, stack, BRK/RTI, IRQ and NMI.
- All official addressing modes, zero-page wrap, page penalties and indirect-JMP bug.
- Instruction-level cycle accounting and deterministic illegal-opcode errors.
- Safe flat-binary loader, headless runner, optional SDL2 frontend.
- Original ca65 Snake game with pause, persistent high score, and a reproducible `make snake` target.
- Strict debug, sanitizer and dependency-free test targets.

## Remaining hardening

1. Run the Klaus Dormann decimal suite and fix any discovered edge cases.
2. Expand table-driven tests to cover every official opcode/address-mode pairing.
3. Add a trace/disassembler runner for diagnosing external ROM failures.

The official Klaus Dormann functional suite passes at `$3469` after 30,646,176
instructions. Linux CI covers strict, sanitizer, functional-ROM, Snake, and SDL
UI builds.

## Future platforms

- Undocumented NMOS opcodes.
- Audio, save states and an interactive debugger.
- A real historical machine such as Apple I or NES. NES support is a separate major
  project requiring the PPU, APU, controllers, cartridge parsing/mappers and tighter timing.
