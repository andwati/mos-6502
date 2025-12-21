# design doc
The MOS 6502 is an 8-bit microprocessor with a 16-bit address bus, providing a linear addressable memory space of 64KiB

Unlike its contemporaries, the 6502 was designed with a minimal internal register set to reduce transistor count and manufacturing costs, featuring only one primary 8-bit accumulator (A), two 8-bit index registers (X and Y), a 16-bit program counter (PC), and an 8-bit stack pointer (S). 
This lack of general-purpose registers is compensated for by the implementation of "Zero Page" addressing, which allows the first 256 bytes of memory ($0000–$00FF) to be treated as a large, fast-access register file

The architectural philosophy of the 6502 is rooted in the Von Neumann model, where instructions and data share the same memory bus. A critical aspect of 6502 emulation is the adherence to little-endian byte ordering, meaning the least significant byte of a 16-bit address is stored at the lower memory address

# Primary Internal Register Specifications

The internal state of the 6502 is defined by a specific set of registers that dictates the processor's operational capabilities.

| Register Name    | Mnemonic | Width(Bits) | Functional Description                                          |
| ---------------- | -------- | ----------- | --------------------------------------------------------------- |
| Accumulator      | A        | 8           | The primary register for all arithmetic and logical operations. |
| Index Register X | X        | 8           | Used primarily for indexed addressing and as a loop counter.    |
| Index Register Y | Y        | 8           | Used primarily for indexed addressing and pointer offsets.      |
| Program Counter  | PC       | 16          | Stores the address of the next instruction to be fetched.       |
| Stack Pointer    | S        | 8           | Points to the next free location in the $0100-$01FF page.       |
| Status Register  | P        | 8           | Holds the processor flags (N, V, D, I, Z, C).                   |


# Processor Status Register (P) Detail

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

# memory bus and adressing subsystem
The 6502 interacts with the world through its 16-bit address bus. In the initial phase, memory is implemented as a flat 64KiB array of uint8_t. However, to accommodate future hardware mapping (ROM, RAM, and I/O), all memory access must be abstracted through read and write functions.

## Memory Map and Reserved Vectors
The emulator must respect specific address ranges hard-coded into the 6502's silicon.

| Address Range | Component  | Description                                   |
| ------------- | ---------- | --------------------------------------------- |
| $0000 – $00FF | Page Zero  | Reserved for fast, single-byte addressing.    |
| $0100 – $01FF | Page One   | Hard-wired system stack region.               |
| $FFFA – $FFFB | NMI Vector | Points to the Non-Maskable Interrupt handler. |
| $FFFC – $FFFD | RES Vector | Points to the Power-On Reset address.         |
| $FFFE – $FFFF | IRQ Vector | Points to the Interrupt Request/BRK handler.  |

## Memory Access Implementation
Effective 6502 emulation requires a read_byte and write_byte interface. This allows the implementation of memory-mapped I/O (MMIO), where reading from a specific address (e.g., $F004) might poll a keyboard instead of returning a RAM value

This design decision ensures that the CPU logic is completely decoupled from the physical memory layout. For instance, if mirroring is required where the same memory appears at multiple addresses due to incomplete address decoding this logic is contained within the bus module.

# CPU State Initialization and the Fetch Cycle
The CPU is modeled as a C struct containing all registers and flags. This encapsulation supports multiple CPU instances and provides a clean interface for testing.


## The Reset Sequence
Upon power-up or a reset signal, the 6502 does not begin execution at a random address. It fetches a 16-bit address from the Reset Vector at $FFFC/$FFFD and sets the PC to that value. The stack pointer is often set to $FF, and the interrupt flag is initialized to disabled.

# Opcode Decoding and Instruction Dispatch
The core of the emulator is the execution loop, which performs the Fetch-Decode-Execute sequence. Every 6502 instruction begins with a 1-byte opcode. There are 256 possible opcodes, though only 56 are officially documented. 

## Dispatch Decision: Switch vs. Jump Tables
A critical design decision is how to dispatch these 256 opcodes. Three primary methods exist in C:

1. Giant Switch Statement: A 256-case switch block. Most compilers optimize this into a highly efficient jump table. It is readable and allows for easy inlining of small instruction logic.   
1. Array of Function Pointers: An array where `dispatch[opcode]` points to a dedicated function. This is modular but incurs function call overhead and can be slower on modern CPUs due to pointer dereferencing.
1. Computed Gotos: A GCC extension (`goto *labels[opcode]`). This is the fastest for high-performance interpreters but is non-standard.   

For this design document, a Giant Switch Statement is recommended. This approach balances performance, maintainability, and portability, allowing the compiler to perform optimal branch prediction. 

# Addressing Mode Architecture
Addressing modes are the rules for calculating the effective memory address of an operand. Decoupling these from the instruction logic is essential to avoid a combinatorial explosion of code (e.g., LDA has 8 different opcodes for different modes).

## Logical Grouping of Addressing Modes
| Mode        | Abbr | Bytes | Logic                                      |
| ----------- | ---- | ----- | ------------------------------------------ |
| Immediate   | IMM  | 2     | Address is `PC++`.                         |
| Zero Page   | ZP   | 2     | Address is `read_byte(PC++)`.              |
| Absolute    | ABS  | 2     | Address is `read_word(PC); PC+=2`.         |
| Zero Page,X | ZPX  | 2     | Address is `(read_byte(PC++) + X) & 0xFF`. |
| Indirect,Y  | INY  | 2     | Read base from ZP; add Y; handle carry.    |

One of the most complex modes is "Indirect, Y-indexed." It fetches a zero-page address, reads a 16-bit pointer from that location, and then adds the Y register to the pointer. If the addition of Y causes the address to cross a page boundary (e.g., $01FF to $0200), an additional clock cycle is incurred.

# Implementation of the Instruction Set
With the bus and addressing modes in place, the specific instruction logic can be implemented. Most instructions update the Zero (Z) and Negative (N) flags based on the result.

## Data Transfer and Logic
These instructions are the "bread and butter" of the 6502, used to move data between memory and registers or perform bitwise operations.   

- LDA (Load Accumulator): Fetches a byte from the calculated address, stores it in A, and updates N and Z.   

- STA (Store Accumulator): Writes register A to memory.   

- AND/ORA/EOR: Bitwise operations between memory and the accumulator

# The Arithmetic Logic Unit (ALU)
The Add with Carry (ADC) and Subtract with Carry (SBC) instructions are widely regarded as the most difficult to implement correctly. They must handle the Carry flag (for multi-byte math), the Overflow flag (for signed math), and the Decimal flag (for BCD math).

## Binary Mode ADC Logic
The overflow flag (V) is set if the addition of two numbers with the same sign results in a different sign. For example, $127 + 1 = 128$, which in signed 8-bit logic is $-128$ (an overflow).

SBC (Subtract with Carry) is ingeniously implemented by inverting the bits of the operand and calling the ADC logic. This reflects the internal hardware of the 6502, which used a single adder and bit-inverters for subtraction.

# Control Flow, Branches, and the Stack
The 6502 supports subroutines and conditional branching. Subroutines utilize the stack, while branches use relative addressing.

## Stack Operations
The stack pointer (S) is relative to the address space $0100–$01FF. As values are pushed, S decrements.

## Conditional Branching
Branch instructions (e.g., BNE, BEQ) use an 8-bit signed offset to move the PC relative to its current position. A branch takes 2 cycles if not taken, 3 cycles if taken, and 4 cycles if the branch crosses a page boundary. 

# Advanced Decimal Mode (BCD)
The 6502 features a Decimal (D) flag that enables Binary Coded Decimal arithmetic. This allows the processor to perform base-10 addition and subtraction natively ,a critical feature for calculators and accounting software in the 1970s. 

## BCD Algorithm and Behavioral Divergence

The implementation of BCD mode varies significantly between the NMOS 6502 and the CMOS 65C02. On the NMOS version, the Negative, Overflow, and Zero flags are calculated based on the binary result before the decimal adjustment, effectively rendering them meaningless for decimal results. The CMOS version (found in later systems like the Apple IIc) corrected this behavior.

Bruce Clark's algorithm for BCD addition is the gold standard for high-fidelity emulation :

1. Low Nibble Addition: Sum the low nibbles and the carry flag: $AL = (A \& 0x0F) + (B \& 0x0F) + C$
1. Low Nibble Adjustment: If $AL > 9$, set $AL = ((AL + 6) \& 0x0F) + 16$
1. High Nibble Addition: Sum high nibbles with the carry-out from the low nibble: $A = (A \& 0xF0) + (B \& 0xF0) + AL$
1. High Nibble Adjustment: If $A > 0x9F$, set $A = A + 0x60$
1. Final Result: The accumulator result is $A \& 0xFF$, and the Carry flag is set if $A > 0xFF$

# Interrupts and Hardware Interactions
Interrupts allow external devices to pause the CPU's execution. There are three vectors: RESET, NMI (Non-Maskable), and IRQ (Maskable).

## Interrupt Sequence Logic
When an interrupt (NMI or IRQ) is triggered, the CPU performs the following:

1. Pushes PC (High, then Low) to the stack.
1. Pushes the status register P to the stack.
1. Sets the Interrupt Disable (I) flag.
1. Fetches the target address from the appropriate vector ($FFFA/B or $FFFE/F).

# Validation through the Klaus Dormann Test Suite
A 6502 emulator is only "complete" when it passes the Klaus Dormann functional test suite. This suite consists of thousands of lines of assembly code designed to stress-test every edge case, including BCD math and undocumented opcodes

## Integration of the Test Suite
The test suite is provided as a binary image. To run it, the architect must load it at address $0000 and set the PC to $0400
- Success Condition: The program enters an infinite loop at a specific success address, which can be verified by checking the program counter against the assembly listing.
- Failure Analysis: If a failure occurs, the test enters an infinite "trap" loop at a specific address. The developer must then correlate this PC value with the assembly source to identify the failing instruction.

| Test Suite               | Purpose                         | Key Checkpoint                    |
| ------------------------ | ------------------------------- | --------------------------------- |
| 6502_functional_test.bin | Opcode and Flag accuracy.       | Success loop at end of suite.     |
| 6502_decimal_test.bin    | Detailed BCD Math verification. | Validates Bruce Clark algorithms. |
| 6502_interrupt_test.bin  | Timing of IRQ and NMI signals.  | Requires external feedback logic. |
| AllSuiteA.bin            | General compatibility check.    | Checks address $0210 for $FF.     |

# Benchmarking and Performance Metrics
The performance of an emulator is characterized by its execution speed relative to the original hardware. A standard 6502 ran at 1MHz, executing roughly 0.43 million instructions per second.

## Performance Calculation (MIPS)
MIPS (Millions of Instructions Per Second) is calculated by timing the execution of a known number of instructions. 

$$\text{MIPS} = \frac{\text{Instructions Executed}}{\text{Execution Time (s)} \times 10^6}$$

For modern C emulators, typical results on a 3GHz CPU range between 150 and 500 MIPS, meaning the emulator can run the equivalent of a 500MHz 6502 if unthrottled.

# Cycle-Level Timing Accuracy
While an instruction-stepped emulator (Phase 0-10) is sufficient for many tasks, specific systems like the Atari 2600 or NES require "cycle-accurate" timing. In these systems, the CPU and graphics chip (PPU) must be synchronized on a cycle-by-cycle basis. 

## Implementing Cycle Accuracy
To achieve this, the m6502_step function must be modified to return the number of cycles used, or better yet, to "tick" external components during each internal micro-operation.

A common optimization is to use a lookup table for the base cycle counts of each opcode and then add cycles for "penalties" like page boundary crossings or branch completions