# design doc
The MOS 6502 is an 8-bit microprocessor with a 16-bit address bus, providing a linear addressable memory space of 64KiB

Unlike its contemporaries, the 6502 was designed with a minimal internal register set to reduce transistor count and manufacturing costs, featuring only one primary 8-bit accumulator (A), two 8-bit index registers (X and Y), a 16-bit program counter (PC), and an 8-bit stack pointer (S). 
This lack of general-purpose registers is compensated for by the implementation of "Zero Page" addressing, which allows the first 256 bytes of memory ($0000–$00FF) to be treated as a large, fast-access register file

The architectural philosophy of the 6502 is rooted in the Von Neumann model, where instructions and data share the same memory bus. A critical aspect of 6502 emulation is the adherence to little-endian byte ordering, meaning the least significant byte of a 16-bit address is stored at the lower memory address

# primary register specification

The internal state of the 6502 is defined by a specific set of registers that dictates the processor's operational capabilities.

| Register Name    | Mnemonic | Width(Bits) | Functional Description                                          |
| ---------------- | -------- | ----------- | --------------------------------------------------------------- |
| Accumulator      | A        | 8           | The primary register for all arithmetic and logical operations. |
| Index Register X | X        | 8           | Used primarily for indexed addressing and as a loop counter.    |
| Index Register Y | Y        | 8           | Used primarily for indexed addressing and pointer offsets.      |
| Program Counter  | PC       | 16          | Stores the address of the next instruction to be fetched.       |
| Stack Pointer    | S        | 8           | Points to the next free location in the $0100-$01FF page.       |
| Status Register  | P        | 8           | Holds the processor flags (N, V, D, I, Z, C).                   |


# processor status register(P) detail
The status register is a collection of binary flags that represent the outcome of recent operations or control the processor's mode of operation.

| Bit | Flag | Name      | Description                                             |
| --- | ---- | --------- | ------------------------------------------------------- |
| 7   | N    | Negative  | Set if the result of an operation has bit 7 set.        |
| 6   | V    | Overflow  | Set if signed arithmetic results in an invalid sign.    |
| 5   | \-   | Unused    | Always read as 1 on some NMOS implementations.          |
| 4   | B    | Break     | Set only when a BRK instruction pushes status to stack. |
| 3   | D    | Decimal   | Toggles Binary Coded Decimal (BCD) mode for ADC/SBC.    |
| 2   | I    | Interrupt | Masks IRQ requests when set.                            |
| 1   | Z    | Zero      | Set if the result of an operation is zero.              |
| 0   | C    | Carry     | Set if an operation results in a carry or borrow.       |