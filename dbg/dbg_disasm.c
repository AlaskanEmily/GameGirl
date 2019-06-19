#include "dbg.h"

#include "mmu.h"

/* Dummy out the actual instructions */
#include "cpu_dummy.h"

/* get snprintf */
#include <stdio.h>

#undef GG_ILLEGAL

#define GG_ILLEGAL( N ) \
    return "ILLEGAL " #N;

#define GG_OPCODE(N, BYTES, CYCLES) case N: \
    in_out_address[0] += (BYTES);

/* Used for all the IMM8 opcodes */
#define GG_OPCODE_FMT_IMM8(FMT) \
    snprintf(out, 79, (FMT), (unsigned)GG_Read8MMU(mmu, start_address+1)); return out;

/* Used for all the IMM16 opcodes */
#define GG_OPCODE_FMT_IMM16(FMT) \
    snprintf(out, 79, (FMT), (unsigned)GG_Read16MMU(mmu, start_address+1)); return out;

#define GG_OPCODE_FMT8 "0x%0.2X"
#define GG_OPCODE_FMT16 "0x%0.4X"

#define GG_OPCODE_NO_ARG( N1 ) \
    return #N1;

#define GG_OPCODE_REG8_REG8( N1, N2, N3 ) \
    return #N1 " " #N2 ", " #N3;

#define GG_OPCODE_REG16_REG16(N1, N2, N3) \
    return #N1 " " #N2 ", " #N3;

#define GG_OPCODE_REG16_IMM8( N1, N2 ) \
    GG_OPCODE_FMT_IMM8( #N1 " " #N2 ", " GG_OPCODE_FMT8 )

#define GG_OPCODE_IMM8_REG16( N1, N2 ) \
    GG_OPCODE_FMT_IMM8(#N1 " " GG_OPCODE_FMT8 ", " #N2)

#define GG_OPCODE_REG16_IMM16( N1, N2 ) \
    GG_OPCODE_FMT_IMM16( #N1 " " #N2 ", " GG_OPCODE_FMT16 )

#define GG_OPCODE_IMM16_REG16( N1, N2 ) \
    GG_OPCODE_FMT_IMM16( #N1 " " GG_OPCODE_FMT16 ", " #N2 )

#define GG_OPCODE_REG8_IMM8( N1, N2 ) \
    GG_OPCODE_FMT_IMM8( #N1 " " #N2 ", " GG_OPCODE_FMT8 )

#define GG_OPCODE_IMM8_REG8( N1, N2 ) \
    GG_OPCODE_FMT_IMM8(#N1 " " GG_OPCODE_FMT8 ", " #N2)

#define GG_OPCODE_REGPTR_REG8( N1, N2, N3 ) \
    return #N1 " [" #N2 "], " #N3;

#define GG_OPCODE_REG8_REGPTR( N1, N2, N3 ) \
    return #N1 " " #N2 ", [" #N3 "]";

#define GG_OPCODE_REGPTR_IMM8( N1, N2 ) \
    GG_OPCODE_FMT_IMM8(#N1 " [" #N2 "], " GG_OPCODE_FMT8)

#define GG_OPCODE_IMM8_REGPTR( N1, N2 ) \
    GG_OPCODE_FMT_IMM8(#N1 " " GG_OPCODE_FMT8 ", [" #N2 "]")

#define GG_OPCODE_REG8_IMMPTR( N1, N2 ) \
    GG_OPCODE_FMT_IMM16(#N1 " " #N2 ", [" GG_OPCODE_FMT16 "]")

#define GG_OPCODE_IMMPTR_REG8( N1, N2 ) \
    GG_OPCODE_FMT_IMM16(#N1 " [" GG_OPCODE_FMT16 "], " #N2)

#define GG_OPCODE_REG16_IMMPTR( N1, N2 ) \
    GG_OPCODE_FMT_IMM16(#N1 " " #N2 ", [" GG_OPCODE_FMT16 "]")

#define GG_OPCODE_IMMPTR_REG16( N1, N2 ) \
    GG_OPCODE_FMT_IMM16(#N1 " [" GG_OPCODE_FMT16 "], " #N2)

#define GG_OPCODE_IMM8HIPTR_REG8( N1, N2 ) \
    GG_OPCODE_FMT_IMM8(#N1 " [0xFF%0.2X], " #N2)

#define GG_OPCODE_REG8_IMM8HIPTR( N1, N2 ) \
    GG_OPCODE_FMT_IMM8(#N1 " " #N2 ", [0xFF%0.2X]")

#define GG_OPCODE_REG8HIPTR_REG8( N1, N2, N3 ) \
    GG_OPCODE_FMT_IMM8(#N1 " [0xFF00+" #N2 "], " #N3)

#define GG_OPCODE_REG8_REG8HIPTR( N1, N2, N3 ) \
    GG_OPCODE_FMT_IMM8(#N1 " " #N2 ", [0xFF00+" #N3 "]")

#define GG_OPCODE_REGPTR( N1, N2 ) \
    return #N1 " [" #N2 "]";

#define GG_OPCODE_REG16( N1, N2 ) \
    return #N1 " " #N2;

#define GG_OPCODE_REG8( N1, N2 ) \
    return #N1 " " #N2;

#define GG_OPCODE_REGA( N1 ) \
    return #N1 " a";

#define GG_OPCODE_REGA_REG8( N1, N2 ) \
    return #N1 " a, " #N2;

#define GG_OPCODE_REGA_IMM8( N1 ) \
    GG_OPCODE_FMT_IMM8(#N1 " a, " GG_OPCODE_FMT8);

#define GG_OPCODE_REGA_REGPTR( N1, N2 ) \
    return #N1 " a, [" #N2 "]";

#define GG_OPCODE_IMM8( N1 ) \
    GG_OPCODE_FMT_IMM8(#N1 " " GG_OPCODE_FMT8)

#define GG_OPCODE_SIMM8( N1 ) \
    { \
        const signed char c = GG_Read8MMU(mmu, start_address+1);\
        snprintf(out, 79, #N1 " " GG_OPCODE_FMT16, (*in_out_address)+c); \
        return out; \
    }

#define GG_OPCODE_IMM16( N1 )\
    GG_OPCODE_FMT_IMM16(#N1 " " GG_OPCODE_FMT16)

#define GG_OPCODE_ABSOLUTE( N1, N2 ) \
    return #N1 " " #N2;

#define GG_END_OPCODE( N )

#define GG_PREFIX_CB() \
    in_out_address[0]++; \
    return "BITOP (NOT IMPLEMENTED!)";

const char *GG_DebugDisassemble(void *mmu,
    unsigned *in_out_address,
    char out[80]){
    
    const unsigned start_address = *in_out_address;
    const unsigned char op = GG_Read8MMU(mmu, start_address);
    switch(op){
#include "cpu.inc"
        default: return "UNKNOWN";
    }
}
