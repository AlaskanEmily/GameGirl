/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_CPU_DUMMY_META_H
#define GG_CPU_DUMMY_META_H
#pragma once

/* Dummies out all of the metadata in cpu.inc
 * This is used by the CPU emulator
 */
#define GG_OPCODE_NO_ARG( N1 )
#define GG_OPCODE_REG16_REG16( N1, N2, N3 )
#define GG_OPCODE_REG8_REG8( N1, N2, N3 )
#define GG_OPCODE_REG16_IMM8( N1, N2 )
#define GG_OPCODE_IMM8_REG16( N1, N2 )
#define GG_OPCODE_REG16_IMM16( N1, N2 )
#define GG_OPCODE_IMM16_REG16( N1, N2 )
#define GG_OPCODE_REG8_IMM8( N1, N2 )
#define GG_OPCODE_IMM8_REG8( N1, N2 )
#define GG_OPCODE_REGPTR_REG8( N1, N2, N3 )
#define GG_OPCODE_REG8_REGPTR( N1, N2, N3 )
#define GG_OPCODE_REGPTR_IMM8( N1, N2 )
#define GG_OPCODE_IMM8_REGPTR( N1, N2 )
#define GG_OPCODE_REG8_IMMPTR( N1, N2 )
#define GG_OPCODE_IMMPTR_REG8( N1, N2 )
#define GG_OPCODE_REG16_IMMPTR( N1, N2 )
#define GG_OPCODE_IMMPTR_REG16( N1, N2 )
#define GG_OPCODE_IMM8HIPTR_REG8( N1, N2 )
#define GG_OPCODE_REG8_IMM8HIPTR( N1, N2 )
#define GG_OPCODE_REG8HIPTR_REG8( N1, N2, N3 )
#define GG_OPCODE_REG8_REG8HIPTR( N1, N2, N3 )
#define GG_OPCODE_REGPTR( N1, N2 )
#define GG_OPCODE_REG16( N1, N2 )
#define GG_OPCODE_REG8( N1, N2 )
#define GG_OPCODE_REGA( N1 ) /* Indicates an opcode which is only used with a */
#define GG_OPCODE_REGA_REG8( N1, N2 ) /* Indicates an opcode which is only used with a */
#define GG_OPCODE_REGA_IMM8( N1 ) /* Indicates an opcode which is only used with a */
#define GG_OPCODE_REGA_REGPTR( N1, N2 ) /* Indicates an opcode which is only used with a */
#define GG_OPCODE_IMM8( N1 )
#define GG_OPCODE_SIMM8( N1 )
#define GG_OPCODE_IMM16( N1 )
#define GG_OPCODE_ABSOLUTE( N1, N2 )

#endif /* GG_CPU_DUMMY_META_H */
