/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_DBG_CORE_H
#define GG_DBG_CORE_H
#pragma once

/*****************************************************************************/

#include "../gg_call.h"

/*****************************************************************************/
/* Debugger core. This is used to communicate breakpoints and state to the CPU,
 * and as an interface from the debugger UI to the CPU/MMU.
 */

#ifdef __cplusplus
#define GG_DBG_FUNC(T) extern "C" GG_STDCALL(T)
#else
#define GG_DBG_FUNC GG_STDCALL
#endif

/*****************************************************************************/

typedef const char *gg_dbg_str_ptr;

/*****************************************************************************/

#define GG_DBG_PAUSE 0
#define GG_DBG_CONTINUE 1

/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/

extern const unsigned gg_dbg_core_struct_size;
extern const unsigned _gg_dbg_core_struct_size;

/*****************************************************************************/

extern const char *const *const gg_dbg_register_names;
extern const char *const *const _gg_dbg_register_names;

/*****************************************************************************/

#ifdef __cplusplus
} // extern "C"
#endif

/*****************************************************************************/

struct GG_DBG_s;
typedef struct GG_DBG_s GG_DBG;
typedef GG_DBG* GG_DBG_ptr;

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_Init(GG_DBG *dbg, void *cpu, void *mmu);

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_Fini(GG_DBG *dbg);

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_SetState(GG_DBG *dbg, int state);

/*****************************************************************************/

GG_DBG_FUNC(int) GG_DBG_GetState(const GG_DBG *dbg);

/*****************************************************************************/

/* The state flag is the first byte in the debugger core. Fun fact. */
#define GG_DBG_GET_STATE(DBG) ((int)(*(unsigned char*)(DBG)))
#define GG_DBG_SET_STATE(DBG, STATE) \
    do{ ((unsigned char*)(DBG))[0] = (STATE); } while(0)

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_SetBreakpoint(GG_DBG *dbg, unsigned address);

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_UnsetBreakpoint(GG_DBG *dbg, unsigned address);

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_UnsetAllBreakpoints(GG_DBG *dbg);

/*****************************************************************************/
/* Returns zero for no breakpoint, non-zero if there is a breakpoint */
GG_DBG_FUNC(int) GG_DBG_IsBreakpoint(GG_DBG *dbg, unsigned address);

/*****************************************************************************/

GG_DBG_FUNC(unsigned) GG_DBG_AddressToLine(const GG_DBG *dbg, unsigned);

/*****************************************************************************/

GG_DBG_FUNC(unsigned) GG_DBG_LineToAddress(const GG_DBG *dbg, unsigned);

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_SetAddress8(GG_DBG *dbg, unsigned addr, unsigned val);

/*****************************************************************************/

GG_DBG_FUNC(unsigned) GG_DBG_GetAddress8(const GG_DBG *dbg, unsigned addr);

/*****************************************************************************/

GG_DBG_FUNC(void) GG_DBG_SetRegister(GG_DBG *dbg, const char *r, unsigned val);

/*****************************************************************************/

GG_DBG_FUNC(unsigned) GG_DBG_GetRegister(const GG_DBG *dbg, const char *r);

/*****************************************************************************/

GG_DBG_FUNC(gg_dbg_str_ptr) GG_DBG_Disassemble(void *mmu,
    unsigned *in_out_address,
    char out[80]);

/*****************************************************************************/

#ifdef __cplusplus
} // extern "C"
#endif

/*****************************************************************************/

#endif /* GG_DBG_CORE_H */
