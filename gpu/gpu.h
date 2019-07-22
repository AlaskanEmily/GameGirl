/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_GPU_GPU_H
#define GG_GPU_GPU_H
#pragma once

#include "../gg_call.h"

#ifdef __cplusplus
#define GG_GPU_FUNC(T) extern "C" GG_STDCALL(T)
#else
#define GG_GPU_FUNC GG_STDCALL
#endif

#define GG_GPU_CALLBACK GG_STDCALL_CALLBACK

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned gg_gpu_struct_size;
extern const unsigned _gg_gpu_struct_size;

#ifdef __cplusplus
} // extern "C"
#endif

struct GG_GPU_s;
typedef struct GG_GPU_s GG_GPU;
typedef GG_GPU *GG_GPU_ptr;

#define GG_GPU_HBLANK_MODE 0
#define GG_GPU_VBLANK_MODE 1
#define GG_GPU_OAM_MODE 2
#define GG_GPU_VRAM_MODE 3

GG_GPU_FUNC(void) GG_GPU_Init(GG_GPU *gpu);
GG_GPU_FUNC(void) GG_GPU_Fini(GG_GPU *gpu);

GG_GPU_FUNC(unsigned char) GG_GPU_GetMode(GG_GPU *gpu);
GG_GPU_FUNC(void) GG_GPU_SetMode(GG_GPU *gpu, unsigned char mode);

GG_GPU_FUNC(unsigned char) GG_GPU_GetLine(GG_GPU *gpu);
GG_GPU_FUNC(void) GG_GPU_SetLine(GG_GPU *gpu, unsigned char line);

GG_GPU_FUNC(unsigned) GG_GPU_GetModeClock(GG_GPU *gpu);
GG_GPU_FUNC(void) GG_GPU_SetModeClock(GG_GPU *gpu, unsigned modeclock);

typedef GG_GPU_CALLBACK(void, on_gpu_advance_callback)(void *arg);

/* Returns the current mode */
GG_GPU_FUNC(unsigned) GG_GPU_Advance(GG_GPU *gpu,
    void *win,
    void *mmu,
    unsigned clocks,
    on_gpu_advance_callback cb,
    void *cb_arg);

/* The GPU components have a guaranteed ABI on x86.
 * This helps a lot on less optimizing compilers in cpu.c
 */
#if ((defined __i386) || (defined _M_IX86)) && (!defined GG_NO_GPU_MACROS)

#define GG_GPU_DATA(GPU, TYPE, BYTE_I) \
    ((TYPE*)(((unsigned char *)(GPU))+BYTE_I))

#define GG_GPU_GETMODE(GPU) (*GG_GPU_DATA(GPU, unsigned char, 0))
#define GG_GPU_SETMODE(GPU, ARG) (*GG_GPU_DATA(GPU, unsigned char, 0) = (ARG))
#define GG_GPU_GETLINE(GPU) (*GG_GPU_DATA(GPU, unsigned char, 1))
#define GG_GPU_SETLINE(GPU, ARG) (*GG_GPU_DATA(GPU, unsigned char, 1) = (ARG))
#define GG_GPU_GETMODECLOCK(GPU) (*GG_GPU_DATA(GPU, unsigned, 4))
#define GG_GPU_SETMODECLOCK(GPU, ARG) (*GG_GPU_DATA(GPU, unsigned, 4) = (ARG))

/* This is safe to use as a macro on any architecture, but it's only really
 * useful on x86. It tries to avoid diving into GG_GPU_Advance if possible.
 */
#define GG_GPU_ADVANCE(GPU, WIN, MMU, CLOCK, CB, ARG) \
    do{ \
        const unsigned GG_GPU_ADVANCE_mode = GG_GPU_GETMODE(GPU);\
        const unsigned GG_GPU_ADVANCE_modeclock = \
            GG_GPU_GETMODECLOCK(GPU) + (CLOCK);\
        if( (GG_GPU_ADVANCE_mode == 0 && GG_GPU_ADVANCE_modeclock >= 816) || \
            (GG_GPU_ADVANCE_mode == 1 && GG_GPU_ADVANCE_modeclock >= 1824) || \
            (GG_GPU_ADVANCE_mode == 2 && GG_GPU_ADVANCE_modeclock >= 320) || \
            (GG_GPU_ADVANCE_mode == 3 && GG_GPU_ADVANCE_modeclock >= 688)) { \
            GG_GPU_Advance(GPU, WIN, MMU, CLOCK, CB, ARG); \
        } \
        else {\
            GG_GPU_SETMODECLOCK(GPU, GG_GPU_ADVANCE_modeclock);\
        } \
    }while(0)

#else

#define GG_GPU_GETMODE GG_GPU_GetMode
#define GG_GPU_SETMODE GG_GPU_SetMode
#define GG_GPU_GETLINE GG_GPU_SetLine
#define GG_GPU_SETLINE GG_GPU_GetLine
#define GG_GPU_GETMODECLOCK GG_GPU_GetModeClock
#define GG_GPU_SETMODECLOCK GG_GPU_SetModeClock
#define GG_GPU_ADVANCE GG_GPU_Advance

#endif

#endif /* GG_GPU_GPU_H */
