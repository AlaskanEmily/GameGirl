/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_DBG_UI_H
#define GG_DBG_UI_H
#pragma once

/*****************************************************************************/

#include "../gg_call.h"

/*****************************************************************************/
/* Debugger UI. This controls the debugger window, and uses has a totally
 * different implementation depending on the platform.
 */

#ifdef __cplusplus
#define GG_DBG_UI_FUNC(T) extern "C" GG_STDCALL(T)
#else
#define GG_DBG_UI_FUNC GG_STDCALL
#endif

/*****************************************************************************/

struct GG_DBG_s;

/*****************************************************************************/

struct GG_DBG_UI_s;
typedef struct GG_DBG_UI_s GG_DBG_UI;
typedef GG_DBG_UI *GG_DBG_UI_ptr;

/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/

extern const unsigned gg_dbg_ui_struct_size;
extern const unsigned _gg_dbg_ui_struct_size;

/*****************************************************************************/

#ifdef __cplusplus
} // extern "C"
#endif

/*****************************************************************************/

GG_DBG_UI_FUNC(void) GG_InitDebuggerWindowSystem(void);

/*****************************************************************************/

GG_DBG_UI_FUNC(void) GG_DBG_UI_Init(GG_DBG_UI *ui, struct GG_DBG_s *dbg);

/*****************************************************************************/

GG_DBG_UI_FUNC(void) GG_DBG_UI_Fini(GG_DBG_UI *ui);

/*****************************************************************************/
/* Updates the debugger window to contain current values from the core.
 * This the only point where the window will read from the core.
 * This should be called before entering a breakpoint and after performing any
 * manipulations that the window/user requests.
 */
GG_DBG_UI_FUNC(void) GG_DBG_UI_Update(GG_DBG_UI *ui);

/*****************************************************************************/
/* Process any pending events from the debugger window.
 * Returns non-zero if the window has been closed.
 * This should be called regularly (such as on flipscreen).
 */
GG_DBG_UI_FUNC(int) GG_DBG_UI_HandleEvents(GG_DBG_UI *ui);

/*****************************************************************************/
/* This must be called after calling GG_PollDebuggerWindow, but before
 * GG_UpdateDebuggerWindow. The caller (usually the debugger core?) must
 * fill out the debugger UI with the disassembly for these lines.
 *
 * Returns non-zero if there are any changes (hopefully there usually aren't?)
 */
GG_DBG_UI_FUNC(int) GG_DBG_UI_NeededLines(const GG_DBG_UI *win,
    unsigned *out_start,
    unsigned *out_end);

/*****************************************************************************/

#endif /* GG_DBG_UI_H */
