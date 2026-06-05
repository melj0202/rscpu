# rscpu

A software simulator for the RSCPU instruction set, written in C++. Built from scratch for a computer architecture course to develop a hands-on understanding of how CPUs work at the hardware level — instruction fetch/decode/execute cycles, register behavior, memory access, and cache design.

---

## What It Does

rscpu loads a bytecode program from a file, dumps it into a 64KB simulated memory space, and executes it instruction by instruction through a fetch-decode-execute loop. It supports a full instruction set with both 8-bit and 16-bit variants, a flag register, conditional branching, and a direct-mapped cache with hit/miss tracking.

---

## Architecture

### Fetch-Decode-Execute Loop
The simulator runs a classic CPU pipeline loop: fetch the next instruction from memory into the Instruction Register (IR), advance the Program Counter (PC), decode the opcode, and dispatch to the appropriate handler function.

### Registers
| Register | Width | Purpose |
|---|---|---|
| AC | 16-bit | Accumulator — primary arithmetic register |
| R | 16-bit | Secondary general-purpose register |
| DR | 8-bit | Data register — holds values read from memory |
| TR | 8-bit | Temporary register |
| IR | 8-bit | Instruction register — holds current opcode |
| AR | 16-bit | Address register — holds memory address for current operation |
| PC | 16-bit | Program counter |
| flag | 8-bit | Status flags: Zero (Z), Carry (C), Overflow (V), Negative (N) |

### Instruction Set
The simulator implements a complete instruction set with both 8-bit and 16-bit versions of most operations:

- **Memory:** LDAC, STAC, MVI (load, store, move immediate)
- **Register:** MVAC, MOVR (move between AC and R)
- **Arithmetic:** ADD, SUB, INAC, CLAC (with correct carry/overflow detection)
- **Logical:** AND, OR, XOR, NOT
- **Shifts/Rotates:** LSL, LSR, RL, RR (logical shifts and circular rotates)
- **Branches:** JUMP, JMPZ, JPNZ, JMPC, JV, JN (conditional jumps based on flags)
- **Control:** NOP, HALT

### Flag Handling
Each arithmetic and logical operation correctly updates the Zero, Negative, Carry, and Overflow flags. Signed overflow and unsigned carry are detected separately — for example, ADD checks whether the result crossed the signed boundary (overflow) independently of whether it wrapped the unsigned boundary (carry).

### Direct-Mapped Cache
The simulator includes a **1KB direct-mapped, write-through, write-allocate cache** layered over the simulated memory. All memory reads and writes go through the cache:

- **Direct-mapped:** each memory address maps to exactly one cache line
- **Write-through:** writes update both the cache and main memory simultaneously  
- **Write-allocate:** on a write miss, the relevant memory block is loaded into cache before writing
- Cache hits and misses are tracked and reported at the end of execution

---

## What I Learned

- How a **fetch-decode-execute loop** works at the implementation level — not just conceptually
- How **direct-mapped caches** work: address decomposition into tag, index, and offset bits using bitmasking and shifts
- The difference between **write-through and write-back**, and **write-allocate vs no-write-allocate** cache policies
- How **carry and overflow flags** are correctly computed for both signed and unsigned arithmetic
- How **conditional branching** is implemented — jumping by overwriting the Program Counter
- Bit manipulation in C++ for register operations: masking, shifting, and composing multi-byte values from byte-width components

---

## Building and Running

Requires a C++ compiler (g++ or clang++).

```bash
g++ -o rscpu rscpuecache.cpp
./rscpu
```

When prompted, enter the path to a bytecode program file. Sample programs are included in the `programs/` directory.

### Program File Format
Programs are plain text files where each line contains one hexadecimal byte value. For example:

```
16     # opcode
00     # high byte of address
05     # low byte of address
FF     # HALT
```

---

## Context

Built as coursework for a computer architecture course. The instruction set architecture (RSCPU) was defined by the course; the simulator implementation — registers, memory model, cache, flag logic, fetch-decode loop — was written entirely from scratch.
