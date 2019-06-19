/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_CPU_CPU_H
#define GG_CPU_CPU_H
#pragma once

#ifndef GG_CCALL
#if (defined __GNUC__) || (defined __TINYC__)
#define GG_CCALL(T) __attribute__((cdecl)) T
#elif (defined _MSC_VER) || (defined __WATCOMC__)
#define GG_CCALL(T) T __cdecl
#else
#error Add cdecl for your compiler here.
#endif
#endif

#ifdef __cplusplus
#define GG_CPU_FUNC(T) extern "C" GG_CCALL(T)
#else
#define GG_CPU_FUNC GG_CCALL
#endif

extern const unsigned gg_cpu_struct_size;

struct GG_CPU_s;
typedef struct GG_CPU_s GG_CPU;
typedef GG_CPU* GG_CPU_ptr;

#define GG_REGISTERS_XY( X, R1, R2 ) \
    X( R1 ) \
    X( R2 ) \
    X( R1 ## R2 )

#define GG_ALL_REGISTERS( X ) \
    GG_REGISTERS_XY( X, A, F ) \
    GG_REGISTERS_XY( X, B, C ) \
    GG_REGISTERS_XY( X, D, E ) \
    GG_REGISTERS_XY( X, H, L ) \
    X(SP) \
    X(IP)

/* Register access functions */
#define GG_DECLARE_REGISTER_ACCESS( R ) \
GG_CPU_FUNC(unsigned) GG_CPU_Get ## R(const struct GG_CPU_s *cpu); \
GG_CPU_FUNC(void) GG_CPU_Set ## R(struct GG_CPU_s *cpu, unsigned val);

GG_ALL_REGISTERS( GG_DECLARE_REGISTER_ACCESS )

#undef GG_DECLARE_REGISTER_ACCESS

GG_CPU_FUNC(void) GG_CPU_Init(GG_CPU *cpu, void *mmu);
GG_CPU_FUNC(void) GG_CPU_Execute(GG_CPU *cpu,
    void *mmu,
    void *gpu,
    void *win,
    void *dbg,
    void *render_cb,
    void *render_arg);

#endif /* GG_CPU_CPU_H */
