#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "../common/common.h"

void emulate( uint8_t* mem, uint8_t address_size );

int main( int argc, char** argv )
{
    FILE* infile = NULL;
    uint8_t* raw_mem = NULL;
    
    if ( argc < 2 )
    {
        printf( "Usage: becpu <filename>\n" );
        return 1;
    }

    printf( "Reading file '%s' ...\n", argv[1] );
    
    infile = fopen( argv[1], "rb" );
    if ( !infile )
    {
        printf( "Could not open file %s.\n", argv[1] ); 
        goto error;
    }

    uint8_t header[4];
    if ( !fread( header, 4, 1, infile ))
    {
        printf( "Failed to read header.\n" );
        goto error;
    }

    if ( header[0] != 'B' || header[1] != 'E' )
    {
        printf( "Invalid file header.\n" );
        goto error;
    }

    uint8_t address_size = header[2];

    if ( address_size != 4 && address_size != 12 && address_size != 20 )
    {
        printf( "Invalid address size: %u\n", address_size );
        goto error;
    }

    printf( "Using address size: %u\n", address_size );

    uint32_t mem_size = 1 << address_size;
    raw_mem = calloc( mem_size, 1 );
    if ( !raw_mem )
    {
        printf( "Memory error!\n" );
        goto error;
    }
    uint32_t address, length;
    
    // TODO > Use last byte of the header for output and delay settings.
    while ( fread( &address, 4, 1, infile ))
    {
        // Convert to big-endian, if necessary.
        address = swapbig32( address );

        if ( !fread( &length, 4, 1, infile ))
        {
            if ( feof( infile ))
            {
                printf( "Unexpected end of file.\n" );
            }
            break;
        }

        length = swapbig32( length );

        if ( length + address > mem_size )
        {
            printf( "Memory address falls out of range for this address size.\n" );
            goto error;
        }

        if ( !fread( raw_mem + address, length, 1, infile ))
        {
            if ( feof( infile ))
            {
                printf( "Unexpected end of file.\n" );
            }
            break;
        }
    }

    if ( ferror( infile ))
    {
        printf( "Unknown error occurred in file read.\n" );
        goto error;
    }

    // Done reading.
    fclose( infile );
    infile = NULL;

    FILE* testout = fopen( "dump.bin", "wb" );
    fwrite( raw_mem, mem_size, 1, testout );
    fclose( testout );
    
    printf( "Starting emulation ...\n" );

    emulate( raw_mem, address_size );
    printf( "Done.\n" );

    free( raw_mem );
    raw_mem = NULL;

    return 0;
    
error:
    if ( raw_mem )
    {
        free( raw_mem );
    }
    
    if ( infile )
    {
        fclose( infile );
    }
    return 1;
}

void getopdata( uint8_t* location, uint8_t address_size, BE_Op* opcode, uint32_t* opdata )
{
    *opcode = *location >> 4;
    
    *opdata = 0;
    switch ( address_size )
    {
    case 20:
        *opdata = (*location & 0xF) << 16 | *(location + 1) << 8 | *(location + 2);
        break;
        
    case 12:
        *opdata = (*location & 0xF) << 8 | *(location + 1);
        break;
        
    case 4:
        *opdata |= *location & 0xF;
        break;
    }
}

void emulate( uint8_t* mem, uint8_t address_size )
{
    uint32_t instruction_pointer = 0;
    uint8_t instruction_size = (address_size + 4) / 8;
    uint8_t A;
    
    int done = 0;
    while ( !done )
    {
        BE_Op opcode;
        uint32_t opdata;

        getopdata( mem + instruction_pointer, address_size, &opcode, &opdata );
        instruction_pointer += instruction_size;
        
        switch( opcode )
        {
        case LDA:
            A = mem[opdata];
            break;

        case ADD:
            A += mem[opdata];
            break;

        case SUB:
            A -= mem[opdata];
            break;

        case STA:
            mem[opdata] = A;
            break;

        case LDI:
            A = address_size == 4 ? opdata & 0xF : opdata & 0xFF;
            break;

        case JMP:
            instruction_pointer = opdata;
            break;

        case OUT:
            printf( "%u\n", A );
            fflush( stdout );
            break;

        case HLT:
            done = 1;
            break;

        default:
            // NOP
            ;
        }
    }
}
