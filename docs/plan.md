# Project status

## Implemented

- 64 KiB bus, reset vectors, deterministic RNG and Easy6502 keyboard/framebuffer MMIO.
- All official NMOS 6502 instructions and opcode encodings.
- Binary and NMOS decimal ADC/SBC, status flags, stack, BRK/RTI, IRQ and NMI.
- All official addressing modes, zero-page wrap, page penalties and indirect-JMP bug.
- Instruction-level cycle accounting and deterministic illegal-opcode errors.
- Stable undocumented NMOS opcode families and multi-byte NOP encodings.
- Safe flat-binary loader, headless runner, optional SDL2 frontend.
- Versioned, endian-defined, CRC32-protected portable save states.
- Extensible bus device hooks and an Apple I terminal/PIA target.
- NES Ricoh 2A03 mode, iNES mappers 0/2/3, PPU register/timing model, controller,
  scrolling, masks, 8x8/8x16 sprites, sprite status, pulse/triangle/noise audio,
  SDL frontend, and full 8,991-line nestest match.
- Original ca65 Snake game with pause, persistent high score, and a reproducible `make snake` target.
- Strict debug, sanitizer and dependency-free test targets.

## Validation

- 2,360,000 independent SingleStepTests cases cover every implemented official
  and stable undocumented opcode, final CPU/RAM state, and instruction cycles.
- The interactive debugger supports stepping, continue, breakpoints, register and
  memory inspection/editing, disassembly, and portable save/load.

The official Klaus Dormann functional suite passes at `$3469` after 30,646,176
instructions. Linux CI covers strict, sanitizer, all external CPU/platform suites,
the complete single-step corpus, Snake, and SDL UI builds.

Bruce Clark's exhaustive NMOS decimal suite passes all 131,072 operand/carry
combinations, including invalid BCD digits. Klaus Dormann's interrupt suite
passes IRQ, NMI, BRK, vector, flag, and concurrent-interrupt checks.

## Explicit compatibility boundary

The NES frontend is a useful first-generation NES subsystem, not a claim of full
commercial-library compatibility. Mappers other than 0, 2, and 3, DMC audio,
cycle-by-cycle PPU bus effects, and mapper IRQs remain outside its stated scope.
