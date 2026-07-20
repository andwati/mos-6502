# External validation ROMs

`make fetch-roms` downloads the official Klaus Dormann NMOS 6502 functional
test from the [upstream GPL-3.0 repository](https://github.com/Klaus2m5/6502_65C02_functional_tests).
The download is pinned by SHA-256 and is not committed here.

`make test-functional` fetches and runs it. Success is the documented loop at
`$3469`.
