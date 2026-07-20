# MOS 6502 emulator

A portable NMOS MOS 6502 emulator in C with an Easy6502-compatible virtual
machine and an original Snake game.

The reusable core implements all 56 documented instructions (151 official
opcode encodings), NMOS decimal arithmetic, IRQ/NMI/BRK, stack behavior,
addressing quirks, and instruction cycle counts. Unsupported opcodes stop with
an explicit error.

## Build and test

```sh
make                 # core and command-line emulator
make test            # unit/integration checks
make test-functional # official Klaus Dormann functional suite
make debug            # strict -Werror debug build and tests
make sanitize         # AddressSanitizer/UBSan build and tests
```

The core has no external dependencies. The graphical frontend uses SDL2 and
SDL2_ttf. Building the included game also needs `ca65` and `ld65` from cc65.
On Debian/Ubuntu install `libsdl2-dev`, `libsdl2-ttf-dev`, `fonts-dejavu-core`,
and `cc65`. Set `MOS6502_FONT` to another TrueType font path if desired.

```sh
make snake
```

The startup menu offers Start/Resume, New Game, Reset High Score, and Quit. Use
Up/Down or W/S to select and Enter or Space to activate. During play, WASD or
the arrow keys steer, Space or P pauses, R restarts after game over, and Escape
returns to the menu. D-pad and Start controls are supported on SDL game
controllers. The window title shows the current
score and persistent high score. The high score is stored in
`snake.highscore` in the working directory.

F5 saves the current game and F9 restores it. Food and game-over events have
lightweight synthesized sound effects. Save states are stored in `snake.state`.

The in-game arcade layout includes a framed pixel display, live score and high
score, control legend, responsive resizing, and a full playfield pause overlay.

Run another flat binary with:

```sh
bin/mos6502 program.bin --load 0x0600 --start 0x0600
```

Useful options are `--hz`, `--scale`, `--seed`, `--headless`, and `--trace`.

External functional ROMs can be run without modifying the application:

```sh
make test-rom ROM=6502_functional_test.bin LOAD=0 START=0x400 SUCCESS=0x3469
```

## Virtual machine

The machine provides 64 KiB RAM, a game-state byte at `$00FB`, a high-score byte
at `$00FC`, a game-score byte at `$00FD`, a random byte at `$00FE`, the latest keyboard character at `$00FF`,
and a 32x32 16-color framebuffer at `$0200-$05FF`.
This is deliberately a small game-oriented computer, not an NES emulator.
Original arcade Pac-Man used a Z80; running that ROM would require a different
CPU and hardware platform.

See [the design](docs/design.md) and [project status](docs/plan.md).

## Validation status

The in-tree tests exercise the CPU's major correctness boundaries. External
Klaus Dormann functional and decimal ROM validation is the next hardening gate;
the binaries are not vendored because their redistribution terms must be
respected.
