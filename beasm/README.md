
# Introduction

The `beasm` project is an assembler for the `beem` emulator.

# Usage

`beasm <input_file> <output_file>`

# Syntax

Each .beasm file begins with a single number, specifying the size of an address. May be 4, 12, or 20.

Afterwards, there follows a set of blocks of the following format:

    <address> {
        <data>;
        <data>;
        ...
    }

    Where <data> is either:
        <opcode> <instruction_data>
    OR a byte value in the range 0...255

All numbers may be encoded as hexadecimal by prefixing them with `0x`, or binary by prefixing them with `0b`.

# Examples

None yet :D
