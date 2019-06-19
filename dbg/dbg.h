/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_DBG_H
#define GG_DBG_H
#pragma once

/*****************************************************************************/

#ifdef __TINYC__
#define GG_DEBUG_CALL(T) __attribute__((cdecl)) T
#elif defined __GNUC__ && defined __i386
#define GG_DEBUG_CALL(T) __attribute__((cdecl, visibility("default"))) T
#else
#define GG_DEBUG_CALL(T) T __cdecl
#endif

/*****************************************************************************/

typedef const char *gg_dbg_str_ptr;

/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/* Debugger Core components. */
/*****************************************************************************/

#define GG_DBG_PAUSE 0
#define GG_DBG_CONTINUE 1

/*****************************************************************************/

struct GG_Debugger;
typedef struct GG_Debugger *GG_DebuggerPtr;
typedef const struct GG_Debugger *GG_ConstDebuggerPtr;

GG_DEBUG_CALL(GG_DebuggerPtr) GG_CreateDebugger(void *cpu, void *mmu);

/*****************************************************************************/

GG_DEBUG_CALL(void) GG_DestroyDebugger(struct GG_Debugger *dbg);

/*****************************************************************************/

GG_DEBUG_CALL(int) GG_GetDebuggerState(const struct GG_Debugger *dbg);

/*****************************************************************************/

GG_DEBUG_CALL(void) GG_SetDebuggerState(struct GG_Debugger *dbg, int);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_DebuggerAddressToLine(const struct GG_Debugger *dbg, unsigned);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_DebuggerLineToAddress(const struct GG_Debugger *dbg, unsigned);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_DebuggerSetAddress8(struct GG_Debugger *dbg, unsigned addr, unsigned val);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_DebuggerGetAddress8(const struct GG_Debugger *dbg, unsigned addr);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_DebuggerSetRegister(struct GG_Debugger *dbg, const char *r, unsigned val);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_DebuggerGetRegister(const struct GG_Debugger *dbg, const char *r);

/*****************************************************************************/
/* Debugger UI elements. */
/*****************************************************************************/

struct GG_DebuggerUI;
typedef struct GG_DebuggerUI *GG_DebuggerUIPtr;
typedef const struct GG_DebuggerUI *GG_ConstDebuggerUIPtr;

/*****************************************************************************/

struct GG_DebuggerUILine;
typedef struct GG_DebuggerUILine *GG_DebuggerUILinePtr;
typedef const struct GG_DebuggerUILine *GG_ConstDebuggerUILinePtr;

/*****************************************************************************/

struct GG_DebuggerUIBreak;
typedef struct GG_DebuggerUIBreak *GG_DebuggerUIBreakPtr;
typedef const struct GG_DebuggerUIBreak *GG_ConstDebuggerUIBreakPtr;

/*****************************************************************************/

GG_DEBUG_CALL(GG_DebuggerUIPtr) GG_CreateDebuggerUI(void);

/*****************************************************************************/

GG_DEBUG_CALL(void) GG_DestroyDebuggerUI(struct GG_DebuggerUI *ui);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_GetDebuggerUINumBreaks(const struct GG_DebuggerUI *ui);

/*****************************************************************************/

/* Returns zero on success, non-zero on failure */
GG_DEBUG_CALL(int)
GG_SetDebuggerUINumBreaks(struct GG_DebuggerUI *ui, unsigned n);

/*****************************************************************************/

GG_DEBUG_CALL(GG_DebuggerUIBreakPtr)
GG_GetDebuggerUIBreak(struct GG_DebuggerUI *ui, unsigned n);

/*****************************************************************************/

GG_DEBUG_CALL(GG_ConstDebuggerUIBreakPtr)
GG_GetConstDebuggerUIBreak(const struct GG_DebuggerUI *ui, unsigned n);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_SetDebuggerUIBreak(struct GG_DebuggerUIBreak *,
    unsigned addr, unsigned line);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_GetDebuggerUIBreakAddress(const struct GG_DebuggerUIBreak *br);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_SetDebuggerUIBreakAddress(struct GG_DebuggerUIBreak *br, unsigned addr);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_GetDebuggerUIBreakLine(const struct GG_DebuggerUIBreak *br);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_SetDebuggerUIBreakLine(struct GG_DebuggerUIBreak *br, unsigned line);

/*****************************************************************************/
/* Returns zero on success, non-zero on failure */
GG_DEBUG_CALL(int)
GG_SetDebuggerUINumLines(struct GG_DebuggerUI *ui, unsigned lines);

/*****************************************************************************/

GG_DEBUG_CALL(GG_DebuggerUILinePtr)
GG_GetDebuggerUILine(struct GG_DebuggerUI *ui, unsigned line);

/*****************************************************************************/

GG_DEBUG_CALL(GG_ConstDebuggerUILinePtr)
GG_GetConstDebuggerUILine(const struct GG_DebuggerUI *ui, unsigned line);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_SetDebuggerUILineSourceLine(struct GG_DebuggerUILine *line, unsigned l);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_GetDebuggerUILineSourceLine(const struct GG_DebuggerUILine *line);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_SetDebuggerUILineAddress(struct GG_DebuggerUILine *line, unsigned a);

/*****************************************************************************/

GG_DEBUG_CALL(unsigned)
GG_GetDebuggerUILineAddress(const struct GG_DebuggerUILine *line);

/*****************************************************************************/
/* Sets the disassembly/source for this line */
GG_DEBUG_CALL(void)
GG_SetDebuggerUILineText(struct GG_DebuggerUILine *line, const char *text);

/*****************************************************************************/
/* Gets the disassembly/source for this line */
GG_DEBUG_CALL(gg_dbg_str_ptr)
GG_GetDebuggerUILineText(const struct GG_DebuggerUILine *line);

/*****************************************************************************/

GG_DEBUG_CALL(void)
GG_SetDebuggerUILineTextCopy(struct GG_DebuggerUILine *line, const char *text);

/*****************************************************************************/
/* Returns zero on success, non-zero on failure */
GG_DEBUG_CALL(int)
GG_SetDebuggerUILineTextFormat(struct GG_DebuggerUILine *line,
    const char *fmt, ...);
#define GG_DEBUGGER_INVALID_FORMAT 1
#define GG_DEBUGGER_TOO_MANY_FORMATS 2

/*****************************************************************************/
/* Debugger Window elements. */
/*****************************************************************************/

struct GG_DebuggerWindow;
typedef struct GG_DebuggerWindow *GG_DebuggerWindowPtr;

/*****************************************************************************/

GG_DEBUG_CALL(void) GG_InitDebuggerWindowSystem(void);

/*****************************************************************************/

GG_DEBUG_CALL(GG_DebuggerWindowPtr) GG_CreateDebuggerWindow(
    struct GG_DebuggerUI *ui);

/*****************************************************************************/

GG_DEBUG_CALL(void) GG_DestroyDebuggerWindow(struct GG_DebuggerWindow *win);

/*****************************************************************************/
/* Updates the debugger window to contain current values from the DebuggerUI.
 * This the only point where the window will read from the UI component.
 */
GG_DEBUG_CALL(void) GG_UpdateDebuggerWindow(struct GG_DebuggerWindow *win);

/*****************************************************************************/
/* Process any pending events from the debugger window.
 * Returns non-zero if the window has been closed.
 */
GG_DEBUG_CALL(int) GG_PollDebuggerWindow(struct GG_DebuggerWindow *win);

/*****************************************************************************/
/* This must be called after calling GG_PollDebuggerWindow, but before
 * GG_UpdateDebuggerWindow. The caller (usually the debugger core?) must
 * fill out the debugger UI with the disassembly for these lines.
 *
 * Returns non-zero if there are any changes (hopefully there usually aren't?)
 */
GG_DEBUG_CALL(int)
GG_GetDebuggerWindowNeededLines(const struct GG_DebuggerWindow *win,
    unsigned *out_start, unsigned *out_end);

/*****************************************************************************/
/* Assembly functions */
/*****************************************************************************/

GG_DEBUG_CALL(gg_dbg_str_ptr) GG_DebugDisassemble(void *mmu,
    unsigned *in_out_address, char out[80]);

/*****************************************************************************/

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* GG_DBG_H */
