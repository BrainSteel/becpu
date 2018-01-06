# Introduction

This project is an emulator for Ben Eater's CPU.
A playlist detailing the construction of the CPU may be found on
YouTube [here](https://www.youtube.com/watch?v=HyznrdDSSGM&list=PLowKtXNTBypGqImE405J2565dvjafglHU).  
I am not affiliated with Ben Eater, but appreciate his work. I claim no rights to
the design or implementation of his system.

# Functional Description
The CPU built by Ben Eater operates on 8-bit numbers and 16 total bytes of RAM. The program is loaded into
RAM starting at address 0, and at startup, execution begins at address 0. Because the amount of RAM is
fairly limited and easily extended in software, the emulator supports up to 1 MB of RAM (2^20 bytes).

The CPU has only one directly accessible register, A.

The emulator executes instructions consisting of a 4-bit identifier followed by a configurable number of bits (4, 12, or 20)
of instruction data. The instruction identifiers are as follows:


|Binary |OPCODE|Description     |
|-------|------|----------------|
|0001	|LDA   |Loads A with the value stored at the memory address pointed to by instruction data. |
|0010   |ADD   |Adds to A the value stored at the memory address pointed to by instruction data, storing the result in A. |
|0011   |SUB   |Subtracts from A the value stored at the memory address pointed to by instruction data, storing the result in A. |
|0100   |STA   |Stores the value A at the memory address pointed to by instruction data. |
|0101   |LDI   |Loads the least significant 8 bits of instruction data into A. If only 4 bits are used, the most significant 4 bits are assumed 0. |
|0110   |JMP   |Sets the instruction pointer to the memory address pointed to by instruction data. |
|1110   |OUT   |Outputs the value of A. Depending on configuration, output may be formatted as 8-bit unsigned, signed (two's complement), binary, or hexadecimal. Newlines are printed after each value. |
|1111   |HLT   |Stops the CPU. In terms of emulation, this exits the program. |
|Other  |NOP   |Does nothing. May be configured to delay for a number of milliseconds indicated by the instruction data. |

# Usage
`becpu <filename>`

# .becpu File Format

(All data is stored as big-endian unless otherwise specified)

     Byte 0-1:   "BE"    
     Byte 2:     The size of an address, in bits. Valid sizes are 4, 12, or 20.
     Byte 3:     (value & 0x03): Preferred output format.
                       0:  Output as unsigned 8-bit integer (e.g. 255).
                       1:  Output as signed (two's complement) 8-bit integer (e.g. -1).
                       2:  Output as binary (e.g. 0b11011011).
                       3:  Output as hexadecimal (e.g. 0xFF).
                 (value & 0x04): Delay setting for NOP.
                       0: Don't delay on NOP.
                       1: Allow delay on NOP.
     Byte 4-:    Block data.

The block data is a sequence of blocks specifying the data to be loaded into RAM
at the start of execution.  
A block is formatted as follows:

    Byte 0-3: Address, as byte offset (unsigned)
    Byte 4-7: Length, in bytes (unsigned)
    Byte 8-:  Exactly 'Length' bytes of data to be loaded into RAM at the specified address.
