#ifndef GG_CPU_DUMMY_H
#define GG_CPU_DUMMY_H
#pragma once

/* Dummies out all of the pseudo-ops for cpu.inc
 * This is used by the assembler and disassembler
 */

#define GG_TMP8( N )
#define GG_END_TMP8( N )
#define GG_TMP16( N )
#define GG_END_TMP16( N )
#define GG_BEGIN_IF_NOT_FLAG( N )
#define GG_END_IF_NOT_FLAG( N )
#define GG_BEGIN_IF_FLAG( N )
#define GG_END_IF_FLAG( N )
#define GG_TIME( N )

#define GG_SET_FLAG( A )
#define GG_SET_FLAG2( A, B )
#define GG_SET_FLAG3( A, B, C )

#define GG_CLEAR_FLAG( A )
#define GG_CLEAR_FLAG2( A, B )
#define GG_CLEAR_FLAG3( A, B, C )

#define GG_NOP( )
#define GG_STOP( )
#define GG_HALT( )

#define GG_DISABLE_INTERRUPTS()
#define GG_ENABLE_INTERRUPTS()
#define GG_LD_IMM16( REG16 )
#define GG_LD_IMM8( REG8 )
#define GG_LD_REGPTR_REG8( REG16, REG8 )
#define GG_LD_REG8_REGPTR( REG8, REGPTR )
#define GG_LD_REGPTR_REG16( REGPTR, REG16 )
#define GG_LD_REG16_REGPTR( REG16, REGPTR )
#define GG_INC_REG16( REG16 )
#define GG_DEC_REG16( REG16 )
#define GG_INC_REG8( REG8 )
#define GG_DEC_REG8( REG8 )
#define GG_ADD_REG8_REG8( REG8A, REG8B )
#define GG_ADD_REG16_REG16( REG16A, REG16B )
#define GG_ADC_REG8_REG8( REG8A, REG8B )
#define GG_ADC_REG16_REG16( REG16A, REG16B )
#define GG_SUB_REG8_REG8( REG8A, REG8B )
#define GG_SUB_REG16_REG16( REG16A, REG16B )
#define GG_SBC_REG8_REG8( REG8A, REG8B )
#define GG_SBC_REG16_REG16( REG16A, REG16B )
#define GG_RRC_REG8( REG8 )
#define GG_RR_REG8( REG8 )
#define GG_RLC_REG8( REG8 )
#define GG_RL_REG8( REG8 )
#define GG_SAVE_SP( )
#define GG_JREL8( REG8 )
#define GG_DAA( )
#define GG_CPL_REG8( REG8 )
#define GG_LD_REG_REG( REGA, REGB )
#define GG_BITOP( REGA, OP )
#define GG_POP_REG16( REG16 )
#define GG_PUSH_REG16( REG16 )
#define GG_JMP_REG16( REG16 )
#define GG_JMP_ABS( A )
#define GG_ILLEGAL( A )
#define GG_LDH_REG8PTR_REG8( REG8PTR, REG8 )
#define GG_LDH_REG8_REG8PTR( REG8, REG8PTR )

#endif /* GG_CPU_DUMMY_H */
