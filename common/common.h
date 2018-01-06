#ifndef COMMON_H
#define COMMON_H

#include "stdint.h"

#define SIZE_ARRAY( arr ) ((sizeof(arr)) / sizeof(arr[0]))

#define PARSE_ERROR_SUCCESS 0
#define PARSE_ERROR_NUM     (1 << 0)
#define PARSE_ERROR_OP      (1 << 1)
#define PARSE_ERROR_LINE    (1 << 2)
#define PARSE_ERROR_BLOCK   (1 << 3)

#define X_OPCODES   \
    X( NOP, 0x0 )   \
    X( LDA, 0x1 )   \
    X( ADD, 0x2 )   \
    X( SUB, 0x3 )   \
    X( STA, 0x4 )   \
    X( LDI, 0x5 )   \
    X( JMP, 0x6 )   \
    X( OUT, 0xE )   \
    X( HLT, 0xF )

typedef enum BE_Op_e
{
#define X( en, val ) en = val,
    X_OPCODES
#undef X
} BE_Op;

typedef struct BE_NamedOp_s
{
    const char* name;
    BE_Op value;
} BE_NamedOp;

const BE_NamedOp be_operators[] =
{
#define X( en, val ) { #en, en },
    X_OPCODES
#undef X
};

typedef struct BE_LineData_s
{
    int is_op;
    union
    {
        struct
        {
            BE_Op opcode;
            uint32_t op_data;
        };

        uint8_t byte;
    };
} BE_LineData;

typedef struct BE_BlockData_s
{
    uint32_t address;
    uint32_t num;
    // TODO > Variable line data length
    BE_LineData lines[512];
} BE_BlockData;

static inline int machinebig( )
{
    short x = 1;
    return !*(char*)&x;
}

static inline uint16_t swap16( uint16_t in )
{
    return ((in << 8) & 0xFF00) |
        ((in >> 8) & 0x00FF);
}


static inline uint32_t swap32( uint32_t in )
{
    return ((in << 24) & 0xFF000000) |
        ((in << 8) & 0x00FF0000) |
        ((in >> 8) & 0x0000FF00) |
        ((in >> 24) & 0x000000FF);
}   

// This function converts to/from big-endian 16-bit numbers
// from/to native 16-bit numbers.
static inline uint16_t swapbig16( uint16_t in )
{
    if ( !machinebig( ))
    {
        return swap16( in );
    }

    return in;
}

// This function converts to/from big-endian 32-bit numbers
// from/to native 32-bit numbers.
static inline uint32_t swapbig32( uint32_t in )
{
    if ( !machinebig( ))
    {
        return swap32( in );
    }

    return in;
}

#endif

