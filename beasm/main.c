#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"

#include "../common/common.h"

int bintou( const char* str, uint32_t* out )
{
    uint32_t value = 0;
    while ( *str )
    {
        if ( *str == '0' )
        {
            value <<= 1;
        }
        else if ( *str == '1' )
        {
            value <<= 1;
            value++;
        }
        else
        {
            return PARSE_ERROR_NUM;
        }

        str++;
    }

    *out = value;
    return PARSE_ERROR_SUCCESS;
}

void trimcomment( char* str )
{
    // Use '#' as the character symbol.
    char* loc = strchr( str, '#' );
    if ( loc )
        *loc = 0;
}

void toupperstr( char* str )
{
    while ( *str != '\0' )
    {
        if ( *str >= 'a' && *str <= 'z' )
        {
            *str = *str - 32;
        }
        str++;
    }
}

int iswhitespace( const char* str )
{
    while ( *str != '\0' )
    {
        if ( !isspace( *str ))
        {
            return 0;
        }
        str++;
    }
    return 1;
}

int parsenum( const char* line, uint32_t* out )
{
    char buf[256];
    if ( sscanf( line, " %[0-9A-Fa-fx]", buf ) == 1 )
    {
        uint32_t value;
        if ( strncmp( buf, "0x", 2 ) == 0 )
        {
            // If the string begins with 0x, treat it as hexadecimal.
            if ( sscanf( buf, "%x",  &value ) != 1 )
            {
                return PARSE_ERROR_NUM;
            }
        }
        else if ( strncmp( buf, "0b", 2 ) == 0 )
        {
            // If the string starts with 0b, treat it as binary.
            int err = bintou( buf + 2, &value );
            if ( err )
            {
                return err;
            }
        }
        // Otherwise, assume it is an unsigned decimal integer.
        else if ( sscanf( buf, "%u", &value ) != 1 )
        {
            return PARSE_ERROR_NUM;
        }

        *out = value;
        return PARSE_ERROR_SUCCESS;
    }
    else
    {
        return PARSE_ERROR_NUM;
    }
}

int parseop( const char* line, BE_Op* out )
{
    char buf[256];
    strcpy( buf, line );
    toupperstr( buf );
    char opbuf[4];
    if ( sscanf( buf, " %3s", opbuf ) == 1 )
    {
        int index;
        for ( index = 0; index < SIZE_ARRAY(be_operators); index++ )
        {
            if ( strcmp( be_operators[index].name, opbuf ) == 0 )
            {
                *out = be_operators[index].value;
                return PARSE_ERROR_SUCCESS;
            }
        }

        printf( "Unrecognized opcode: %s\n", opbuf );
    }

    return PARSE_ERROR_OP;
}

int parseline( const char* str, BE_LineData* out )
{
    const char* segments[2];
    int nseg = 0;
    int prvwhite = 1;
    while ( *str != '\0' )
    {
        if ( prvwhite && !isspace( *str ))
        {
            if ( nseg >= 2 )
            {
                printf( "Expected operator/data pair or single data byte. Found: %s\n", str );
                return PARSE_ERROR_LINE;
            }
            segments[nseg] = str;
            nseg++;
            prvwhite = 0;
        }
        else if ( isspace( *str ))
        {
            prvwhite = 1;
        }

        str++;
    }

    if ( nseg == 2 )
    {
        // Expect an operator and instruction data.
        out->is_op = 1;
        if ( parseop( segments[0], &out->opcode ))
        {
            printf( "Expected valid operator. Found: %s\n", segments[0] );
            return PARSE_ERROR_LINE;
        }

        if ( parsenum( segments[1], &out->op_data ))
        {
            printf( "Expected instruction data. Found: %s\n", segments[1] );
            return PARSE_ERROR_LINE;
        }
    }
    else
    {
        // Could be either an operator with no data, or a number.
        if ( segments[0][0] >= 'A' && segments[0][0] <= 'Z' )
        {
            // Expect an operator.
            if ( parseop( segments[0], &out->opcode ))
            {
                printf( "Expected valid operator. Found: %s\n", segments[0] );
                return PARSE_ERROR_LINE;
            }
            
            out->is_op = 1;
            out->op_data = 0;
        }
        else
        {
            // Expect a number.
            uint32_t value;
            if ( parsenum( segments[0], &value ))
            {
                printf( "Expected byte. Found: %s\n", segments[0] );
                return PARSE_ERROR_LINE;
            }

            out->is_op = 0;
            out->byte = value & 0xFF;
        }
    }

    return PARSE_ERROR_SUCCESS;
}

int parseblock( FILE* file, BE_BlockData* out )
{
    char buf[256];
    fscanf( file, " %255[^{]{", buf );
    if ( parsenum( buf, &out->address ))
    {
        printf( "Expected address. Found: %s\n", buf );
        return PARSE_ERROR_BLOCK;
    }

    out->num = 0;
    char delim;
    int num;
    while ( (num = fscanf( file, "%255[^;}]%c", buf, &delim )) == 2 )
    {
        if ( delim == '}' )
        {
            if ( !iswhitespace( buf ))
            {
                printf( "Expected end of block. Found: %s\n", buf );
                return PARSE_ERROR_BLOCK;
            }
            break;
        }
        else
        {
            if ( parseline( buf, &out->lines[out->num] ))
            {
                printf( "Expected line. Found: %s\n", buf );
                return PARSE_ERROR_BLOCK;
            }

            out->num++;
        }
    }

    if ( num != 2 )
    {
        printf( "Unexpected end of file within block: %s%c\n", buf, delim );
        return PARSE_ERROR_BLOCK;
    }

    return PARSE_ERROR_SUCCESS;
}

int main( int argc, char** argv )
{
    FILE* infile = NULL;
    FILE* outfile = NULL;

    uint8_t* block_raw = NULL;
    
    if ( argc < 3 )
    {
        printf( "Usage: beasm <input_file> <output_file>\n" );
        goto error;
    }

    infile = fopen( argv[1], "r" );
    if ( infile == NULL )
    {
        printf( "Failed to open file: %s\n", argv[1] );
        goto error;
    }

    outfile = fopen( argv[2], "wb" );
    if ( outfile == NULL )
    {
        printf( "Failed to open output file: %s\n", argv[2] );
        goto error;
    }

    char buf[256];
    buf[0] = '\0';
    while( iswhitespace( buf ))
    {
        fgets( buf, 255, infile );
    }
    
    uint32_t address_size;
    if ( parsenum( buf, &address_size ))
    {
        printf( "Failed to parse address size at beginning of file.\n" );
        goto error;
    }
    else if ( address_size != 20 && address_size != 12 && address_size != 4 )
    {
        printf( "Invalid address size. Valid sizes are 20, 12, 4.\n" );
        goto error;
    }
    else
    {
        printf( "Using address size: %u\n", address_size );
    }

    uint8_t addr_size_byte = address_size;

    // TODO > Allow for different output settings.
    uint8_t header[4] = { 'B', 'E', address_size & 0xFF, 0 };
    
    if ( !fwrite( header, 4, 1, outfile ))
    {
        printf( "Failed to write header.\n" );
        goto error;
    }
    
    BE_BlockData block;
    uint32_t block_index = 0;
    // TODO > Inefficient!
    block_raw = malloc( 512 * ((address_size + 4) / 3) + 8 );
    uint8_t* line_data = block_raw + 8;
    while ( !feof( infile ) && !parseblock( infile, &block ))
    {
        *(uint32_t*)block_raw = swapbig32( block.address );

        uint32_t line_index = 0;
        uint32_t byte_index = 0;
        for ( line_index = 0; line_index < block.num; line_index++ )
        {
            if ( block.lines[line_index].is_op )
            {
                line_data[byte_index] = block.lines[line_index].opcode << 4;
                switch ( address_size )
                {
                case 20:
                    line_data[byte_index++] |= (block.lines[line_index].op_data >> 16) & 0xF;
                    line_data[byte_index++] = (block.lines[line_index].op_data >> 8) & 0xFF;
                    line_data[byte_index++] = block.lines[line_index].op_data & 0xFF;
                    break;
                    
                case 12:
                    line_data[byte_index++] |= (block.lines[line_index].op_data >> 8) & 0xF;
                    line_data[byte_index++] = block.lines[line_index].op_data & 0xFF;
                    break;

                case 4:
                    line_data[byte_index++] |= block.lines[line_index].op_data & 0xF;
                    break;
                }
            }
            else
            {
                line_data[byte_index++] = block.lines[line_index].byte;
            }
        }

        *((uint32_t*)block_raw + 1) = swapbig32( byte_index );

        if ( !fwrite( block_raw, byte_index + 8, 1, outfile ))
        {
            printf( "Failed to write block data.\n" );
            goto error;
        }
        else
        {
            printf( "Wrote block %u. Address: 0x%X, Size: %u\n", block_index++, block.address, byte_index );
        }

        // Search for the next non-whitespace character.
        // If found, rewind one place to read a block.
        // If not found, we will be at EOF and feof( infile )
        // will return true.
        char delim;
        if ( fscanf( infile, " %c", &delim ) == 1 )
        {
            fseek( infile, -1, SEEK_CUR );
        }
    }

    printf( "Finished with no errors.\n" );

    free( block_raw );
    fclose( outfile );
    fclose( infile );
    
    return 0;

error:
    if ( block_raw )
    {
        free( block_raw );
    }
    
    if ( outfile )
    {
        fclose( outfile );
    }

    if ( infile )
    {
        fclose( infile );
    }

    return 1;
}
