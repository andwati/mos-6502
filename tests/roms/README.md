# External validation ROMs

`make fetch-roms` downloads the official Klaus Dormann NMOS 6502 functional
test from the [upstream GPL-3.0 repository](https://github.com/Klaus2m5/6502_65C02_functional_tests).
The download is pinned by SHA-256 and is not committed here.

`make test-functional` fetches and runs it. Success is the documented loop at
`$3469`.

The decimal image is Bruce Clark's public-domain test configured by Gopher2600
for NMOS accumulator and N/V/Z/C flag validation. `make test-decimal` runs all
valid and invalid BCD operand combinations and requires `ERROR == 0`.

`make test-interrupt` assembles Klaus Dormann's GPL-3.0 interrupt test with the
upstream bundled AS65 assembler and verifies IRQ, NMI, BRK, vectors, flags, and
concurrent interrupt priority through its success trap at `$06F5`.

`make test-apple1` assembles the original Woz Monitor source maintained in
Jeff Tranter's 6502 repository, boots its `$FF00` reset vector, injects a memory
examine command through the emulated PIA, and verifies the monitor's output.

`make test-nestest` downloads Kevtris's canonical NROM test and Nintendulator
golden log, then compares all 8,991 CPU states and cumulative cycle counts.
