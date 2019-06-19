/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cpu.h"

#include "mmu.h"
#include "gpu.h"

#define GG_SUPER_DEBUG
#ifdef GG_SUPER_DEBUG
#include "dbg.h"
#include <stdio.h>
#endif

#include <assert.h>

/* Dummy out the meta */
#include "cpu_dummy_meta.h"

#define GG_REGISTER(X, Y) \
    union { \
        unsigned short X ## Y ; \
        unsigned short reg; \
        unsigned char array[2]; \
        struct { unsigned short X ## Y ; } reg16; \
        struct { unsigned char Y; unsigned char X; } reg8; \
    } X ## Y

#if (__STDC_VERSION__ >= 201112L)

typedef _Bool gg_bool_t;
#define GG_TRUE true
#define GG_FALSE false

#elif defined __cplusplus

typedef bool gg_bool_t;
#define GG_TRUE true
#define GG_FALSE false

#else

typedef char gg_bool_t;
#define GG_TRUE 1
#define GG_FALSE 0

#endif

struct GG_CPU_s{
    GG_REGISTER(A, F);
    GG_REGISTER(B, C);
    GG_REGISTER(D, E);
    GG_REGISTER(H, L);
    unsigned short SP;
    unsigned short IP;
    unsigned short M;
    gg_bool_t interrupts_enabled;
};

#define GG_AF(CPU) ((CPU)->AF.reg)
#define GG_A(CPU)  ((CPU)->AF.reg8.A)
#define GG_F(CPU)  ((CPU)->AF.reg8.F)
#define GG_BC(CPU) ((CPU)->BC.reg)
#define GG_B(CPU)  ((CPU)->BC.reg8.B)
#define GG_C(CPU)  ((CPU)->BC.reg8.C)
#define GG_DE(CPU) ((CPU)->DE.reg)
#define GG_D(CPU)  ((CPU)->DE.reg8.D)
#define GG_E(CPU)  ((CPU)->DE.reg8.E)
#define GG_HL(CPU) ((CPU)->HL.reg)
#define GG_H(CPU)  ((CPU)->HL.reg8.H)
#define GG_L(CPU)  ((CPU)->HL.reg8.L)
#define GG_SP(CPU) ((CPU)->SP)
#define GG_IP(CPU) ((CPU)->IP)

#define GG_ZERO_FLAG 0x80
#define GG_OPERATION_FLAG 0x40
#define GG_HALF_CARRY_FLAG 0x20
#define GG_CARRY_FLAG 0x10

#define GG_REGISTER_ACCESS( R ) \
unsigned GG_CPU_Get ## R(const struct GG_CPU_s *cpu){ \
    return GG_ ## R( cpu ); \
} \
void GG_CPU_Set ## R(struct GG_CPU_s *cpu, unsigned val){ \
    GG_ ## R( cpu ) = val; \
}

GG_ALL_REGISTERS( GG_REGISTER_ACCESS )


/* Creates the "TMP" 8-bit register for local use by our pseudo-op file */
#define GG_TMP8( N ) \
    unsigned char tmp[N];\
    DEBUG_ONLY( const char debug_tmp_8 ## N = N );

/* Unused in C, we just do a validation that GG_TMP8 was set first */
#define GG_END_TMP8( N ) \
    (void)(tmp[N-1]); \
    assert(sizeof(tmp) == N); \
    assert(debug_tmp_8 ## N == N);

/* Creates the "TMP" 16-bit register for local use by our pseudo-op file */
#define GG_TMP16( N ) \
    unsigned short tmp[N];\
    DEBUG_ONLY( const char debug_tmp_16 ## N = N );

/* Unused in C, we just do a validation that GG_TMP8 was set first */
#define GG_END_TMP16( N ) \
    (void)(tmp[N-1]); \
    assert(sizeof(tmp) == ((N) << 1)); \
    assert(debug_tmp_16 ## N == N);

/* Mocks to stand in for the register names when using TMP's */
#define GG_TMP0(CPU) tmp[0]
#define GG_TMP1(CPU) tmp[1]
#define GG_TMP2(CPU) tmp[2]
#define GG_TMP3(CPU) tmp[3]

const unsigned gg_cpu_struct_size = sizeof(struct GG_CPU_s);

#ifdef NDEBUG
#define DEBUG_ONLY(X)
#else
#define DEBUG_ONLY(X) X
#endif

/* Macros to turn the cpu.inc into C code */

#define GG_OPCODE(N, BYTES, CYCLES) case N: \
    DEBUG_ONLY(debug_op = N); \
    m += (CYCLES); \
    {

#define GG_END_OPCODE(N) \
    } \
    assert(debug_op == N); \
    break;

/* Macros for the instructions. */

#define GG_NOP()

/* TODO! */
#define GG_STOP()
#define GG_HALT()
#define GG_PREFIX_CB() \
    m += 8; \
    gg_prefix_cb(cpu, mmu, GG_Read8MMU(mmu, ip++));

/* TODO: Some of these are actually BCD opcodes */
#define GG_ILLEGAL( N ) assert(debug_op == N); assert( 0 && "Illegal instruction!" );

/* TODO! */
#define GG_ENABLE_INTERRUPTS( ) \
    cpu->interrupts_enabled = GG_TRUE;

#define GG_DISABLE_INTERRUPTS( ) \
    cpu->interrupts_enabled = GG_FALSE;

/* Set a flag */
#define GG_SET_FLAG( FLAG_NAME ) \
    GG_F( cpu ) |= GG_ ## FLAG_NAME ## _FLAG;

/* Set two flags */
#define GG_SET_FLAG2( FLAG_NAME1, FLAG_NAME2 ) \
    GG_F( cpu ) |= (GG_ ## FLAG_NAME1 ## _FLAG | GG_ ## FLAG_NAME2 ## _FLAG);

/* Set three flags */
#define GG_SET_FLAG3( FLAG_NAME1, FLAG_NAME2, FLAG_NAME3 ) \
    GG_F( cpu ) |= (GG_ ## FLAG_NAME1 ## _FLAG | \
        GG_ ## FLAG_NAME2 ## _FLAG | \
        GG_ ## FLAG_NAME3 ## _FLAG);

/* Clear a flag */
#define GG_CLEAR_FLAG( FLAG_NAME ) \
    GG_F( cpu ) &= ~(GG_ ## FLAG_NAME ## _FLAG);

/* Clear two flags */
#define GG_CLEAR_FLAG2( FLAG_NAME1, FLAG_NAME2 ) \
    GG_F( cpu ) &= ~(GG_ ## FLAG_NAME1 ## _FLAG | GG_ ## FLAG_NAME2 ## _FLAG);

/* Clear three flags */
#define GG_CLEAR_FLAG3( FLAG_NAME1, FLAG_NAME2, FLAG_NAME3 ) \
    GG_F( cpu ) &= ~(GG_ ## FLAG_NAME1 ## _FLAG | \
        GG_ ## FLAG_NAME2 ## _FLAG | \
        GG_ ## FLAG_NAME3 ## _FLAG);

/* Load 16-bit immediate into register */
#define GG_LD_IMM16( REG16 ) \
        GG_ ## REG16(cpu) = GG_Read16MMU(mmu, ip); \
        ip += 2;

/* Load 8-bit immediate into register */
#define GG_LD_IMM8( REG8 ) \
        GG_ ## REG8(cpu) = GG_Read8MMU(mmu, ip); \
        ++ip;

/* Load from register into pointer register */
#define GG_LD_REGPTR_REG8( REGPTR, REG8 ) \
    GG_Write8MMU( mmu, GG_ ## REGPTR( cpu ), GG_ ## REG8( cpu ) );

/* Load from pointer register into register */
#define GG_LD_REG8_REGPTR( REG8, REGPTR ) \
    GG_ ## REG8( cpu ) = GG_Read8MMU( mmu, GG_ ## REGPTR( cpu ) );

/* Load from register into pointer register */
#define GG_LD_REGPTR_REG16( REGPTR, REG16 ) \
    GG_Write16MMU( mmu, GG_ ## REGPTR( cpu ), GG_ ## REG16( cpu ) );

/* Load from pointer register into register */
#define GG_LD_REG16_REGPTR( REG16, REGPTR ) \
    GG_ ## REG16( cpu ) = GG_Read16MMU( mmu, GG_ ## REGPTR( cpu ) );

#define GG_LDH_REG8PTR_REG8( REG8PTR, REG8 ) \
    { \
        unsigned short addr = GG_ ## REG8PTR( cpu ); \
        GG_Write8MMU( mmu, addr | 0xFF00, GG_ ## REG8( cpu ) ); \
    }

#define GG_LDH_REG8_REG8PTR( REG8, REG8PTR ) \
    { \
        unsigned short addr = GG_ ## REG8PTR( cpu ); \
        GG_ ## REG8( cpu ) = GG_Read8MMU( mmu, addr | 0xFF00 ); \
    }

/* Load from pointer register into register */
#define GG_LD_REG_REG( REGA, REGB ) \
    GG_ ## REGA( cpu ) = GG_ ## REGB( cpu );

/* Increment 16-bit register */
#define GG_INC_REG16( REG16 ) \
    ++ GG_ ## REG16( cpu );

/* Decrement 16-bit register */
#define GG_DEC_REG16( REG16 ) \
    -- GG_ ## REG16( cpu );

/* Complement 8-bit register */
#define GG_CPL_REG8( REG8 ) \
    GG_ ## REG8( cpu ) ^= 0xFF; \
    GG_F( cpu ) |= GG_OPERATION_FLAG|GG_HALF_CARRY_FLAG;

/* Increment 8-bit register */
#define GG_INC_REG8( REG8 ) \
    { \
        const unsigned char r8 = GG_ ## REG8( cpu ); \
        unsigned char flags = GG_F( cpu ); \
        flags &= ~(GG_ZERO_FLAG|GG_HALF_CARRY_FLAG|GG_OPERATION_FLAG); \
        if((r8 & 0x0F) == 0x0F){ \
            flags |= GG_HALF_CARRY_FLAG; \
            if(r8 == 0xFF) \
                flags |= GG_ZERO_FLAG; \
        } \
        GG_F( cpu ) = flags; \
        GG_ ## REG8( cpu ) = r8 + 1; \
    }
    
/* Decrement 8-bit register */
#define GG_DEC_REG8( REG8 ) \
    { \
        const unsigned char r8 = GG_ ## REG8( cpu ); \
        unsigned char flags = GG_F( cpu ); \
        flags &= ~(GG_ZERO_FLAG|GG_HALF_CARRY_FLAG); \
        if((r8 & 0x0F) == 0){ \
            flags |= GG_HALF_CARRY_FLAG; \
        } \
        else if(r8 == 1) \
            flags |= GG_ZERO_FLAG; \
        GG_F( cpu ) = flags | GG_OPERATION_FLAG; \
        GG_ ## REG8( cpu ) = r8 - 1; \
    }

/* Rotate left "with carry". This looks wrong, but it matches some docs... */
#define GG_RLC_REG8( REG8 ) \
    { \
        const unsigned char c = GG_ ## REG8( cpu ); \
        if(c & 0x80){ \
            GG_F( cpu ) = GG_CARRY_FLAG; \
            GG_ ## REG8( cpu ) = 1 | (c << 7); \
        } \
        else{ \
            GG_F( cpu ) = 0; \
            GG_ ## REG8( cpu ) = (c << 7); \
        } \
    }

/* Rotate left "with carry". This looks wrong, but it matches some docs... */
#define GG_RRC_REG8( REG8 ) \
    { \
        const unsigned char c = GG_ ## REG8( cpu ); \
        if(c & 1){ \
            GG_F( cpu ) = GG_CARRY_FLAG; \
            GG_ ## REG8( cpu ) = 0x80 | (c >> 1); \
        } \
        else{ \
            GG_F( cpu ) = 0; \
            GG_ ## REG8( cpu ) = (c >> 1); \
        } \
    }

/* Rotate left "without carry". This looks wrong, but it matches some docs... */
#define GG_RL_REG8( REG8 ) \
    { \
        const unsigned char c = GG_ ## REG8( cpu ); \
        const unsigned char f = GG_F( cpu ); \
        GG_F( cpu ) = (c & 0x80) ? GG_CARRY_FLAG : 0;\
        if(f & GG_CARRY_FLAG){ \
            GG_ ## REG8( cpu ) = 1 | (c << 7); \
        } \
        else{ \
            GG_ ## REG8( cpu ) = (c << 7); \
        } \
    }

/* Rotate right "without carry". This looks wrong, but it matches some docs... */
#define GG_RR_REG8( REG8 ) \
    { \
        const unsigned char c = GG_ ## REG8( cpu ); \
        const unsigned char f = GG_F( cpu ); \
        GG_F( cpu ) = (c & 1) ? GG_CARRY_FLAG : 0;\
        if(f & GG_CARRY_FLAG){ \
            GG_ ## REG8( cpu ) = 0x80 | (c >> 1); \
        } \
        else{ \
            GG_ ## REG8( cpu ) = (c >> 1); \
        } \
    }

#define GG_SHIFT_REG8_INNER( REG8, TYPE, CARRYMASK, SHIFTOP ) \
    { \
        TYPE c = GG_ ## REG8( cpu );\
        register unsigned char f = GG_F( cpu ); \
        if(c & CARRYMASK) { \
            f |= GG_CARRY_FLAG; \
        } \
        else { \
            f &= ~GG_CARRY_FLAG; \
        } \
        c SHIFTOP 1;\
        if(c == 0) {\
            f |= GG_ZERO_FLAG; \
        } \
        else { \
            f &= ~GG_ZERO_FLAG; \
        } \
        f &= ~(GG_OPERATION_FLAG|GG_HALF_CARRY_FLAG);\
        GG_F( cpu ) = f; \
        GG_ ## REG8( cpu ) = c; \
    }

#define GG_SLA_REG8( REG8 ) \
    GG_SHIFT_REG8_INNER( REG8, signed char, 0x80, <<= )

#define GG_SRL_REG8( REG8 ) \
    GG_SHIFT_REG8_INNER( REG8, unsigned char, 1, >>= )

#define GG_SRA_REG8( REG8 ) \
    { \
        unsigned char c = GG_ ## REG8( cpu );\
        register unsigned char f = GG_F( cpu ); \
        c >>= 1;\
        if(c == 0) {\
            f |= GG_ZERO_FLAG; \
        } \
        else { \
            f &= ~GG_ZERO_FLAG; \
        } \
        f &= ~(GG_OPERATION_FLAG|GG_HALF_CARRY_FLAG|GG_CARRY_FLAG);\
        GG_F( cpu ) = f; \
        GG_ ## REG8( cpu ) = c; \
    }

#define GG_SWAP_REG8( REG8 ) \
    { \
        const unsigned c = GG_ ## REG8( cpu ); \
        if(c == 0) {\
            unsigned char f = GG_F( cpu );\
            f |= GG_ZERO_FLAG; \
            f &= ~(GG_OPERATION_FLAG|GG_HALF_CARRY_FLAG|GG_CARRY_FLAG); \
            GG_F( cpu ) = f; \
        } \
        else { \
            GG_F( cpu ) &= ~(GG_OPERATION_FLAG| \
                GG_HALF_CARRY_FLAG| \
                GG_CARRY_FLAG| \
                GG_ZERO_FLAG); \
        } \
        GG_ ## REG8( cpu ) = (c >> 4) | (c << 4); \
    }

/* Save stack pointer to immediate address */
#define GG_SAVE_SP() \
    const unsigned short imm = GG_Read16MMU(mmu, ip); \
    ip += 2; \
    GG_Write16MMU(mmu, imm, GG_SP( cpu ));

/* Start of a block which will execute if a flag is set */
#define GG_BEGIN_IF_FLAG( FLAG_NAME ) \
    if( (GG_F( cpu ) & (GG_ ## FLAG_NAME ## _FLAG)) ) {

/* End of a block which will execute if a flag is set */
#define GG_END_IF_FLAG( FLAG_NAME ) }

/* Start of a block which will execute if a flag is not set */
#define GG_BEGIN_IF_NOT_FLAG( FLAG_NAME ) \
    if( !(GG_F( cpu ) & (GG_ ## FLAG_NAME ## _FLAG)) ) {

/* End of a block which will execute if a flag is not set */
#define GG_END_IF_NOT_FLAG( FLAG_NAME ) }

/* Pop 16-bit register from the stack */
#define GG_POP_REG16( REG16 ) \
    GG_ ## REG16( cpu ) = GG_Read16MMU(mmu, GG_SP( cpu )); \
    GG_SP( cpu ) += 2;

/* Push 16-bit register from the stack */
#define GG_PUSH_REG16( REG16 ) \
    GG_Write16MMU(mmu, GG_SP( cpu ), GG_ ## REG16( cpu )); \
    GG_SP( cpu ) -= 2;

/* Jump to 16-bit register */
#define GG_JMP_REG16( REG16 ) \
    GG_IP( cpu ) = GG_ ## REG16( cpu );

#define GG_JMP_ABS( VAL ) \
    GG_IP( cpu ) = ( VAL );

#define GG_TIME( TIME ) m += TIME;

/* Add a 8-bit register to another 8-bit register */
#define GG_ADD_REG8_REG8( REG8_A, REG8_B) \
    GG_ADD_REG8_REG8_INNER( REG8_A, REG8_B, 1)

/* Add a 8-bit register and the carry flag to another 8-bit register */
#define GG_ADC_REG8_REG8( REG8_A, REG8_B) \
    { \
        const unsigned char carry = (GG_F( cpu ) & GG_CARRY_FLAG) ? 1 : 0; \
        GG_ADD_REG8_REG8_INNER( REG8_A, REG8_B, carry) \
    }

/* Used to implement ADD and ADC */
#define GG_ADD_REG8_REG8_INNER( REG8_A, REG8_B, X ) \
    { \
        const unsigned char a = GG_ ## REG8_A( cpu ); \
        const unsigned char b = GG_ ## REG8_B( cpu ); \
        const unsigned short half_carry = (a & 0x0F) + (b & 0x0F) + (X); \
        const unsigned short result = a + b + (X); \
        unsigned char flags = GG_F( cpu ) & ~(GG_OPERATION_FLAG); \
        if( half_carry > 0x0F){ \
            flags |= GG_HALF_CARRY_FLAG; \
        } \
        if( result > 0x00FF){ \
            flags |= GG_CARRY_FLAG; \
        } \
        if(result == 0){ \
            flags |= GG_ZERO_FLAG; \
        } \
        GG_F( cpu ) = flags; \
        GG_ ## REG8_A( cpu ) = (unsigned char)result; \
    }

/* Add a 16-bit register to another 16-bit register */
#define GG_ADD_REG16_REG16( REG16_A, REG16_B ) \
    { \
        const unsigned a = GG_ ## REG16_A( cpu ); \
        const unsigned b = GG_ ## REG16_B( cpu ); \
        const unsigned short half_carry = (a & 0x0FFF) + (b & 0x0FFF); \
        const unsigned result = a + b; \
        unsigned char flags = GG_F( cpu ) & ~(GG_OPERATION_FLAG); \
        if( half_carry > 0x0FFF){ \
            flags |= GG_HALF_CARRY_FLAG; \
        } \
        if( result > 0xFFFF){ \
            flags |= GG_CARRY_FLAG; \
        } \
        GG_F( cpu ) = flags; \
        GG_ ## REG16_A( cpu ) = (unsigned short)result; \
    }

/* Subtract a 8-bit register to another 8-bit register */
#define GG_SUB_REG8_REG8( REG8_A, REG8_B) \
    GG_SUB_REG8_REG8_INNER( REG8_A, REG8_B, 1)

/* Subtract a 8-bit register and the carry flag to another 8-bit register */
#define GG_SBC_REG8_REG8( REG8_A, REG8_B) \
    { \
        const unsigned char carry = (GG_F( cpu ) & GG_CARRY_FLAG) ? 1 : 0; \
        GG_SUB_REG8_REG8_INNER( REG8_A, REG8_B, carry) \
    }

/* Used to implement SUB and SBC */
#define GG_SUB_REG8_REG8_INNER( REG8_A, REG8_B, X ) \
    { \
        const unsigned char a = GG_ ## REG8_A( cpu ); \
        const unsigned char b = GG_ ## REG8_B( cpu ); \
        const unsigned short half_carry = (a & 0x0F) - (b & 0x0F) - (X); \
        const unsigned short result = a - b - (X); \
        unsigned char flags = GG_F( cpu ) & ~(GG_OPERATION_FLAG); \
        if( half_carry > 0x0F){ \
            flags |= GG_HALF_CARRY_FLAG; \
        } \
        if( result > 0x00FF){ \
            flags |= GG_CARRY_FLAG; \
        } \
        if(result == 0){ \
            flags |= GG_ZERO_FLAG; \
        } \
        GG_F( cpu ) = flags | GG_OPERATION_FLAG; \
        GG_ ## REG8_A( cpu ) = (unsigned char)result; \
    }

/* Subtract a 16-bit register to another 16-bit register */
#define GG_SUB_REG16_REG16( REG16_A, REG16_B ) \
    { \
        const unsigned a = GG_ ## REG16_A( cpu ); \
        const unsigned b = GG_ ## REG16_B( cpu ); \
        const unsigned short half_carry = (a & 0x0FFF) - (b & 0x0FFF); \
        const unsigned result = a - b; \
        unsigned char flags = GG_F( cpu ); \
        if( half_carry > 0x0FFF){ \
            flags |= GG_HALF_CARRY_FLAG; \
        } \
        if( result > 0xFFFF){ \
            flags |= GG_CARRY_FLAG; \
        } \
        GG_F( cpu ) = flags | GG_OPERATION_FLAG; \
        GG_ ## REG16_A( cpu ) = (unsigned short)result; \
    }

/* Implementation of bitops. */
#define GG_BITOP_AND &=
#define GG_BITOP_OR |=
#define GG_BITOP_XOR ^=

#define GG_BITOP( REG, OP ) \
    if((GG_ ## REG( cpu ) GG_BITOP_ ## OP GG_ ## REG( cpu )) == 0) \
        GG_F( cpu ) |= GG_ZERO_FLAG; \
    else \
        GG_F( cpu ) &= ~GG_ZERO_FLAG;

/* Relative jump */
#define GG_JREL8( REG8 ) \
    ip += (signed char)GG_ ## REG8( cpu );

/* BCD opcodes
 * These have simple implementations (sort of) on x86, since hilariously enough
 * the Intel 8080 heritage of both the z80 and the 8086 gives them equivalent
 * implementations.
 */
#if (defined __GNUC__ || defined __TINYC__) && (defined __i386 || defined _M_IX86)

#define GG_DAA() \
    { \
        unsigned char flags = GG_F( cpu ); \
        unsigned char in_flags = flags & GG_OPERATION_FLAG; \
        __asm__ ( \
            "movb %1, %%al \n" \
            "btrw $0x05, %%ax \n" \
            "rcr $0x04, %%al \n" \
            "and $0xEE, %%al \n" \
            "shl $0x08, %%ax \n" \
            "sahf \n" \
            "movb %0, %%al \n" \
            "daa \n" \
            "movb %%al, %0 \n" \
            "setz %%ah \n" \
            "setc %%al \n" \
            "shl $0x04, %%al \n" \
            "shr $0x04, %%ax \n" \
            "movb %%al, %1 \n" \
        : "+m"(GG_A( cpu )), "+r"(flags) \
        : \
        : "eax","cc" ); \
        GG_F( cpu ) = flags | in_flags; \
    }
    
#elif (defined __WATCOMC__) && (defined _M_IX86)

void gg_daa_wat(unsigned short *af);
#pragma aux gg_daa_wat = \
"mov al, [edx]" \
"btr ax, 5" \
"rcr al, 4" \
"and al, 0xEE" \
"shl eax, 8" \
"sahf" \
"inc edx" \
"mov al, [edx]" \
"daa" \
"setz ah" \
"setc al" \
"shl al, 4" \
"shr ax, 4" \
"mov [edx], al" \
"dec edx" \
modify [eax] \
parm [edx];

#define GG_DAA()\
    gg_daa_wat( &GG_AF( cpu ) );

#else

/* TODO! */
#define GG_DAA()
/* #error Implement the BCD opcodes! */

#endif

/* TODO: Should this also be pseudo ops? */
void gg_prefix_cb(GG_CPU *const cpu, GG_MMU *const mmu, const unsigned char bz){
    volatile unsigned char b = bz;
    const unsigned char src_type = b & 0x07;
    const unsigned char op_category = b >> 6;
    
    GG_TMP8( 1 )
    
    /* Get the value */
    switch(src_type){
        case 0x0: GG_TMP0( cpu ) = GG_B( cpu ); break;
        case 0x1: GG_TMP0( cpu ) = GG_C( cpu ); break;
        case 0x2: GG_TMP0( cpu ) = GG_D( cpu ); break;
        case 0x3: GG_TMP0( cpu ) = GG_E( cpu ); break;
        case 0x4: GG_TMP0( cpu ) = GG_H( cpu ); break;
        case 0x5: GG_TMP0( cpu ) = GG_L( cpu ); break;
        case 0x6:
            GG_TMP0( cpu ) = GG_Read8MMU(mmu, GG_HL( cpu ));
            break;
        case 0x7: GG_TMP0( cpu ) = GG_A( cpu ); break;
    }
    
    switch(op_category){
        case 0:
            /* A large variety of things... */
            {
                const unsigned char op_val = (b >> 3) & 0x07;
                switch(op_val){
                    case 0: GG_RLC_REG8( TMP0 ) break;
                    case 1: GG_RRC_REG8( TMP0 ) break;
                    case 2: GG_RL_REG8( TMP0 ) break;
                    case 3: GG_RR_REG8( TMP0 ) break;
                    case 4: GG_SLA_REG8( TMP0 ) break;
                    case 5: GG_SRA_REG8( TMP0 ) break;
                    case 6: GG_SWAP_REG8( TMP0 ) break;
                    case 7: GG_SRL_REG8( TMP0 ) break;
                }
            }
            break;
        case 1:
            /* Bit test */
            {
                const unsigned bit = (b >> 3) - 8;
                if( (GG_TMP0( cpu ) & (1<<bit)) != 0 ) {
                    GG_CLEAR_FLAG2( ZERO, OPERATION )
                    GG_SET_FLAG( HALF_CARRY )
                }
                else {
                    GG_CLEAR_FLAG( OPERATION )
                    GG_SET_FLAG2( ZERO, HALF_CARRY )
                }
            }
            break;
        case 2:
            /* Reset bit */
            {
                const unsigned bit = (b >> 3) - 16;
                GG_TMP0( cpu ) &= ~(1<<bit);
            }
            break;
        case 3:
            /* Set bit */
            {
                const unsigned bit = (b >> 3) - 24;
                GG_TMP0( cpu ) |= (1<<bit);
            }
    }
    
    /* Set the value */
    switch(src_type){
        case 0x0: GG_B( cpu ) = GG_TMP0( cpu ); break;
        case 0x1: GG_C( cpu ) = GG_TMP0( cpu ); break;
        case 0x2: GG_D( cpu ) = GG_TMP0( cpu ); break;
        case 0x3: GG_E( cpu ) = GG_TMP0( cpu ); break;
        case 0x4: GG_H( cpu ) = GG_TMP0( cpu ); break;
        case 0x5: GG_L( cpu ) = GG_TMP0( cpu ); break;
        case 0x6:
            GG_Write8MMU(mmu, GG_HL( cpu ), GG_TMP0( cpu ));
            break;
        case 0x7: GG_A( cpu ) = GG_TMP0( cpu ); break;
    }
    
    GG_END_TMP8( 1 )
}

GG_CPU_FUNC(void) GG_CPU_Init(GG_CPU *cpu, void *mmu_v){
    register GG_MMU *const mmu = mmu_v;
    
    GG_AF( cpu ) = 0;
    GG_BC( cpu ) = 0;
    GG_DE( cpu ) = 0;
    GG_HL( cpu ) = 0;
    GG_SP( cpu ) = 0;
    
    cpu->interrupts_enabled = GG_FALSE;
    
    /* Get the entry address */
    if(GG_Read16MMU(mmu, 0x100) == 0xC300){
        cpu->IP = GG_Read16MMU(mmu, 0x102);
    }
    else{
        GG_IP( cpu ) = 0;
    }
}

GG_CPU_FUNC(void) GG_CPU_Execute(GG_CPU *cpu, void *mmu_v, void *gpu_v, void *win_v){
    register unsigned m = cpu->M;
    register unsigned short ip = cpu->IP;
    register GG_MMU *const mmu = mmu_v;
    int count = 0;
    DEBUG_ONLY(int debug_op);
    
    /* ip is now on the C stack */
#undef GG_IP
#define GG_IP(CPU) (ip)
    
    do{
        const unsigned char opcode = GG_Read8MMU(mmu, ip++);
        const unsigned old_m = m;
#if 0 && (defined GG_SUPER_DEBUG)
        {
            unsigned address = ip-1;
            char buffer[80];
            unsigned start_address = address;
            const char *line = GG_DebugDisassemble(mmu, &address, buffer);
            int p = printf("0x%0.4X ", start_address);
            
            p += printf("%s", line);
            
            while(p++ < 31){
                printf(" ");
            }
            do{
                printf(" 0x%0.2X", GG_Read8MMU(mmu, start_address++));
            }while(start_address < address);
            puts(" ");
        }
#endif
        switch(opcode){
#include "cpu.inc"
        }
        
        if(ip == 0xa1)
            printf("B=0x%0.2X\n", GG_B( cpu ));
            
        
        /* Check for interrupts */
        assert(m > old_m);
        GG_GPU_ADVANCE(gpu_v, win_v, mmu, m - old_m);
        if(count++ == 0x1000){
            count = 0;
            printf("IP=0x%0.2X\n", ip);
        }
    }while(1);
}
