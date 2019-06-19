/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dbg.h"

#define __STDC_WANT_LIB_EXT1__ 1
#define WIN32_LEAN_AND_MEAN 1

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#include <Windows.h>

#define GG_TCC_LVM_SETEXTENDEDLISTVIEWSTYLE 0x1036
#define GG_TCC_LVM_INSERTCOLUMNW 0x1061
#define GG_TCC_LVM_DELETEALLITEMS 0x1009
#define GG_TCC_LVM_GETITEMW 0x104B
#define GG_TCC_LVM_SETITEMW 0x104C
#define GG_TCC_LVM_INSERTITEMW 0x104D
#define GG_TCC_WC_LISTVIEWW L"SysListView32"

#ifdef __TINYC__

/* TCC lacks a commdlg header. 
 * This is mostly stuff we don't really need.
 */
struct LVCOLUMNW_s {
    UINT    mask;
    DWORD   fmt;
    DWORD   cx;
    LPWSTR  pszText;
    DWORD   cchTextMax;
    DWORD   iSubItem;
    DWORD   iImage;
    DWORD   iOrder;
};

typedef struct LVCOLUMNW_s LVCOLUMNW;

struct LVITEMW_s {
    UINT    mask;
    DWORD   iItem;
    DWORD   iSubItem;
    UINT    state;
    UINT    stateMask;
    LPWSTR  pszText;
    DWORD   cchTextMax;
    DWORD   iImage;
    LPARAM  lParam;
    DWORD   iIndent;
    DWORD   iGroupId;
    UINT    cColumns;
    PUINT   puColumns;
};

typedef struct LVITEMW_s LVITEMW;

#define WC_LISTVIEWW GG_TCC_WC_LISTVIEWW
#define LVS_REPORT              0x0001
#define LVS_EDITLABELS          0x0200
#define LVS_EX_FULLROWSELECT    0x0020

#define LVCF_FMT                0x0001
#define LVCF_WIDTH              0x0002
#define LVCF_TEXT               0x0004
#define LVCF_SUBITEM            0x0008
#define LVCF_IMAGE              0x0010
#define LVCF_ORDER              0x0020

#define LVCFMT_LEFT             0x0001
#define LVCFMT_RIGHT            0x0002

#define LVM_SETEXTENDEDLISTVIEWSTYLE \
    GG_TCC_LVM_SETEXTENDEDLISTVIEWSTYLE

#define LVM_INSERTCOLUMNW \
    GG_TCC_LVM_INSERTCOLUMNW

#define LVM_DELETEALLITEMS \
    GG_TCC_LVM_DELETEALLITEMS

#define LVM_GETITEMW GG_TCC_LVM_GETITEMW
#define LVM_SETITEMW GG_TCC_LVM_SETITEMW
#define LVM_INSERTITEMW GG_TCC_LVM_INSERTITEMW

#define LVIF_TEXT               0x0001
#define LVIF_IMAGE              0x0002

void WINAPI InitCommonControls(void);

#else
#include <commctrl.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/*****************************************************************************/

#ifndef LVIF_COLUMNS
/* This is needed for TCC and for Watcom */
#define LVIF_COLUMNS 0x0200
#endif

/*****************************************************************************/

#define GG_DEBUGGER_MAX_LINES 1024L

/*****************************************************************************/
/* Contains routines for drawing and interacting with the debugger UI.
 * All general-purpose debugger UI livefs in dbg_ui.c, this is only for the
 * Windows code.
 * This code is primarily used by dbg_ui.c, and is in the dbg_ui.h header
 * instead of dbg.h since it is not meant for general usage.
 */

struct GG_DebuggerWindow{
    HWND win;
    struct GG_DebuggerUI *dbg;
    HANDLE signal_event;
    HANDLE thread;
    /* needed_change is a spinlock used to lock needed_start and needed_end */
    volatile LONG needed_changed, was_closed;
    LONG needed_start, needed_end, line_at; /* line_at is set by update */
};

/*****************************************************************************/
/* Sent when the UI thread should update based on the DebuggerUI data */
static UINT gg_custom_message = 0;

/*****************************************************************************/

#define GG_DEBUGGER_CLASS L"GG_Debugger"

/*****************************************************************************/

#define GG_DEBUGGER_STYLE ( WS_OVERLAPPED | \
    WS_CAPTION | \
    WS_SYSMENU | \
    WS_MINIMIZEBOX | \
    WS_THICKFRAME | \
    WS_MAXIMIZEBOX )

/*****************************************************************************/

#ifndef CW_USEDEFAULT
#define CW_USEDEFAULT 100
#endif

/*****************************************************************************/

#if (defined _MSC_VER) && (_MSC_VER >= 1600)

#define WPRINT_ADDRESS(DEST, ADDR) do{ \
        wchar_t *const WPRINT_ADDRESS_dst = (DEST); \
        assert(WPRINT_ADDRESS_dst != NULL); \
        _snwprintf_s(WPRINT_ADDRESS_dst, 7, _TRUNCATE, L"0x%0.4X", (ADDR)); \
    }while(0)

#elif (defined __WATCOMC__) || (defined _MSC_VER)

/* Watcom and older MSVC still have regular _snwprintf */
#define WPRINT_ADDRESS(DEST, ADDR) do{ \
        wchar_t *const WPRINT_ADDRESS_dst = (DEST); \
        assert(WPRINT_ADDRESS_dst != NULL); \
        _snwprintf(WPRINT_ADDRESS_dst, 7, L"0x%0.4X", (ADDR)); \
        WPRINT_ADDRESS_dst[6] = L'\0'; \
    }while(0)

#elif (defined __STDC_LIB_EXT1__)
/* C11 with Annex K has snwprintf_s, although it really looks more like what
 * snwprintf should...
 */
#define WPRINT_ADDRESS(DEST, ADDR) do{ \
        wchar_t *const WPRINT_ADDRESS_dst = (DEST); \
        assert(WPRINT_ADDRESS_dst != NULL); \
        snwprintf_s(WPRINT_ADDRESS_dst, 7, L"0x%0.4X", (ADDR)); \
        WPRINT_ADDRESS_dst[6] = L'\0'; \
    }while(0)
#else

/* Stupid implementation */
const static wchar_t gg_hex_to_wchar[0xF+1] = {
    L'0', L'1', L'2', L'3',
    L'4', L'5', L'6', L'7',
    L'8', L'9', L'A', L'B',
    L'C', L'D', L'E', L'F'
};

#define WPRINT_ADDRESS(DEST, ADDR) do{ \
        wchar_t *const WPRINT_ADDRESS_dst = (DEST); \
        const unsigned WPRINT_ADDR = (ADDR); \
        assert(WPRINT_ADDRESS_dst != NULL); \
        assert(WPRINT_ADDR <= 0xFFFF); \
        WPRINT_ADDRESS_dst[0] = L'0'; \
        WPRINT_ADDRESS_dst[1] = L'x'; \
        WPRINT_ADDRESS_dst[2] = gg_hex_to_wchar[(WPRINT_ADDR >> 12) & 0xF]; \
        WPRINT_ADDRESS_dst[3] = gg_hex_to_wchar[(WPRINT_ADDR >> 8) & 0xF]; \
        WPRINT_ADDRESS_dst[4] = gg_hex_to_wchar[(WPRINT_ADDR >> 4) & 0xF]; \
        WPRINT_ADDRESS_dst[5] = gg_hex_to_wchar[WPRINT_ADDR & 0xF]; \
        WPRINT_ADDRESS_dst[6] = L'\0'; \
    }while(0)

#endif

/*****************************************************************************/

static LRESULT CALLBACK gg_debugger_window_proc(HWND hwnd,
    UINT msg, WPARAM wparam, LPARAM lparam){
    if(msg == WM_DESTROY || msg == WM_CLOSE || msg == WM_NCDESTROY){
        PostQuitMessage(0);
        ExitThread(0);
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

#define GG_NUM_COLUMNS 4

static DWORD WINAPI gg_debugger_window_thread_proc(void *param){
    
    struct GG_DebuggerWindow *const win = param;
    MSG msg;
    const HINSTANCE instance = GetModuleHandle(NULL);
    
    /* Very fast arena for item text. Since our operations basically consist of
     * invalidating ALL text and then creating new strings, we can just use an
     * arena.
     * Don't allocate this until after we have signalled OK to the main thread.
     * They are waiting on us, it's best not to do unrelated allocations until
     * we unblock them.
     */
    wchar_t *items_text_arena = NULL;
    UINT items_text_arena_at = 0;
    HWND list_view, text_input;
#define GG_ALLOC_ITEM_TEXT(SIZE_IN_WCHARS) \
    items_text_arena+items_text_arena_at; \
    items_text_arena[SIZE_IN_WCHARS] = 0;\
    items_text_arena+=SIZE_IN_WCHARS+1;

#define GG_CLEAR_ALL_ITEM_TEXT() items_text_arena_at = 0
    
    /* TODO:
     * WS_EX_NOACTIVATE to make this always below the emulator?
     */
    win->win = CreateWindowW(GG_DEBUGGER_CLASS,
        L"Debugger",
        GG_DEBUGGER_STYLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600,
        400,
        NULL,
        NULL,
        instance,
        NULL);
    
    win->needed_start = 16;
    win->needed_end = 116;
    win->needed_changed = 1;
    win->was_closed = 0;
    
    /* Report success. */
    if(!SetEvent(win->signal_event)){
		OutputDebugString(TEXT("Could not signal event!"));
		return 0;
	}
    
    items_text_arena = malloc(GG_DEBUGGER_MAX_LINES * 64L);
    
    /* Create the UI elements */
    {
        /* Create the listview used for the disassembly and graphical
         * breakpoint system.
         */
        LVCOLUMNW col;
        list_view = CreateWindowExW(WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"GG_DebuggerListView",
            WS_VISIBLE | WS_CHILD | WS_HSCROLL | LVS_REPORT | LVS_EDITLABELS,
            10,
            10,
            400,
            300,
            win->win,
            NULL,
            instance,
            NULL);
        
        PostMessage(list_view,
            LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
        
        /* Add the columns */
        col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
        col.fmt = LVCFMT_LEFT;
        
        /* Add the address column */
        col.cx = 80;
        col.iSubItem = 0;
        col.pszText = L"Address";
        SendMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)0, (LPARAM)&col);
        
        /* Add the breakpoint column */
        col.cx = 40;
        col.iSubItem = 1;
        col.pszText = L"Break";
        SendMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)1, (LPARAM)&col);
        
        /* Add the assembly code column */
        col.cx = 100;
        col.iSubItem = 2;
        col.pszText = L"Assembly code";
        SendMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)2, (LPARAM)&col);
        
        /* Add the machine code column */
        col.cx = 80;
        col.iSubItem = 3;
        col.pszText = L"Machine code";
        SendMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)3, (LPARAM)&col);
    }
    
    /* Create the debugger text input box */
    text_input = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"Edit", 
        L"",
        WS_VISIBLE | WS_CHILD | ES_LEFT | ES_LOWERCASE,
        10,
        320,
        400,
        20,
        win->win,
        NULL,
        instance,
        NULL);
    PostMessage(text_input, ES_READONLY, FALSE, 0);
    
    ShowWindowAsync(win->win, SW_SHOWNORMAL);
    
    while(GetMessage(&msg, win->win, 0, 0)){
        
        /* Check if we've been signalled to regenerate the list */
        if(msg.message == gg_custom_message){
            register ULONG i;
            const LONG needed_start = win->needed_start;
            ULONG num_lines = win->needed_end - needed_start;
            const struct GG_DebuggerUI *const ui = win->dbg;
            
            UINT gg_column_indices[GG_NUM_COLUMNS] = { 0, 1, 2, 3 };
            LVITEMW item = {
                /* TODO! */ LVIF_COLUMNS|LVIF_TEXT,
                0, /* item number (TO SET) */
                0, /* sub item */
                0, /* state */
                0, /* state mask */
                NULL, /* text (TO SET) */
                0, /* text len (only used when getting info, not setting) */
                0, /* image */
                (LPARAM)0, /* lparam */
                0, /* indent */
                0, /* groupID */
                GG_NUM_COLUMNS, /* num columns */
                NULL /* columns (TO SET) */
#ifndef __TINYC__
              , NULL /* column format */
              , 0 /* group */
#endif
            };
            item.puColumns = gg_column_indices;
            
            assert(win->needed_end >= win->needed_start);
            /* Remove all items. We could do an incremental update, but bleh.
             * This is synchronous (SendMessage versus PostMessage) because
             * we are about to clear all the text data.
             */
            SendMessage(list_view, LVM_DELETEALLITEMS, 0, 0);
            
            GG_CLEAR_ALL_ITEM_TEXT();
            
            /* Add all the lines from the debugger UI */
            for(i = 0; i < num_lines; i++){
                const struct GG_DebuggerUILine *const line =
                    GG_GetConstDebuggerUILine(ui, i);
                
                /* Reset the subitem. */
                item.iSubItem = 0;
                item.iItem = i;
                {
                    /* Send the address, posting the initial item info. */
                    const unsigned address = GG_GetDebuggerUILineAddress(line);
                    item.pszText = GG_ALLOC_ITEM_TEXT(6);
                    WPRINT_ADDRESS(item.pszText, address);
                    SendMessageW(list_view, LVM_INSERTITEMW, 0, (LPARAM)&item);
                }
                
                /* Swap flags to account for subitem info. */
                
                /* TODO! IMAGE ON BREAK COLUMN */
                
                /* Remove the columns info. */
                item.mask = LVIF_TEXT;
                {
                    /* Allocate space for this wide string for the disassembly */
                    const char *const line_text = GG_GetDebuggerUILineText(line);
                    const int line_len =
                        MultiByteToWideChar(CP_UTF8, 0, line_text, -1, NULL, 0) + 1;
                    
                    item.pszText = GG_ALLOC_ITEM_TEXT(line_len);
                    MultiByteToWideChar(CP_UTF8, 0, line_text, -1, item.pszText, line_len);
                    item.iSubItem = 2;
                    SendMessageW(list_view, LVM_SETITEMW, 0, (LPARAM)&item);
                }
                {
                    /* Post a mostly-phony raw view (NOT YET IMPLEMENTED!) */
                    item.pszText = L"0x00";
                    item.iSubItem = 3;
                    SendMessageW(list_view, LVM_SETITEMW, 0, (LPARAM)&item);
                }
            }
            UpdateWindow(list_view);
        }
        
        DispatchMessage(&msg);
    }
    
    DestroyWindow(win->win);
    win->was_closed = 1;
    ExitThread(0);
    return 0;
}

/*****************************************************************************/

static BOOL gg_wait_for_debugger(const struct GG_DebuggerWindow *win){
    HANDLE h[2];
	h[0] = win->signal_event;
    h[1] = win->thread;
    
    if(WaitForMultipleObjects(2, h, FALSE, INFINITE) == WAIT_OBJECT_0){
		ResetEvent(win->signal_event);
		return TRUE;
	}
	else{
		return FALSE;
	}
}

/*****************************************************************************/

void GG_InitDebuggerWindowSystem(void){
    const HANDLE cursor = LoadCursor(NULL, IDC_ARROW);
    WNDCLASSW wc = {
        0,
        gg_debugger_window_proc,
        0,
        0,
        0,
        NULL,
        NULL, /* < TODO! */
        (HBRUSH)(COLOR_BACKGROUND),
        NULL,
        GG_DEBUGGER_CLASS
    };
    
    wc.hCursor = cursor;
    
    RegisterClassW(&wc);
    gg_custom_message = RegisterWindowMessageW(L"GG_DebuggerMessage");
    if(gg_custom_message == 0){
        /* DANGER WILL ROBINSON! */
        OutputDebugStringW(
            L"No available custom window message numbers.");
        gg_custom_message = WM_USER + 3;
    }
    
    InitCommonControls();
    
    assert(LVM_SETEXTENDEDLISTVIEWSTYLE ==
        GG_TCC_LVM_SETEXTENDEDLISTVIEWSTYLE);
    
    assert(LVM_INSERTCOLUMNW ==
        GG_TCC_LVM_INSERTCOLUMNW);
    
    assert(LVM_DELETEALLITEMS == 
        GG_TCC_LVM_DELETEALLITEMS);
    
    assert(LVM_GETITEMW == GG_TCC_LVM_GETITEMW);
    assert(LVM_SETITEMW == GG_TCC_LVM_SETITEMW);
    assert(LVM_INSERTITEMW == GG_TCC_LVM_INSERTITEMW);
    assert(wcscmp(WC_LISTVIEWW, GG_TCC_WC_LISTVIEWW) == 0);
}

/*****************************************************************************/

struct GG_DebuggerWindow *GG_CreateDebuggerWindow(struct GG_DebuggerUI *dbg){
    
    struct GG_DebuggerWindow *const win =
        calloc(1, sizeof(struct GG_DebuggerWindow));
    
    /* Create the signal */
    win->signal_event = CreateEvent(NULL, FALSE, TRUE, NULL);
    
    /* Start the window's thread */
    win->thread = CreateThread(NULL,
        0,
        gg_debugger_window_thread_proc,
        win,
        0,
        NULL);
    
    win->dbg = dbg;
    
    /* Wait for success (or thread explosion) */
    if(gg_wait_for_debugger(win)){
        /* Success! */
        return win;
    }
    else{
        /* Failure */
        CloseHandle(win->signal_event);
        CloseHandle(win->thread);
        free(win);
        return NULL;
    }
}

/*****************************************************************************/

void GG_DestroyDebuggerWindow(struct GG_DebuggerWindow *win){
    DestroyWindow(win->win);
}

/*****************************************************************************/
/* Updates the debugger window to contain current values from the DebuggerUI.
 * This the only point where the window will read from the UI component.
 */
void GG_UpdateDebuggerWindow(struct GG_DebuggerWindow *win){
    PostMessage(win->win, gg_custom_message, 0, 0);
}

/*****************************************************************************/
/* Process any pending events from the debugger window.
 */
int GG_PollDebuggerWindow(struct GG_DebuggerWindow *win){
    /* Nothing to do since we are multithreaded. */
    DWORD result;
    if((!GetExitCodeThread(win->thread, &result)) || result != STILL_ACTIVE){
        CloseHandle(win->thread);
        return 1;
    }
    else{
        return 0;
    }
}

/*****************************************************************************/

int GG_GetDebuggerWindowNeededLines(const struct GG_DebuggerWindow *win_c,
    unsigned *out_start, unsigned *out_end){
    
    /* Remove the constness...very evil, but needed for the spinlock. */
    struct GG_DebuggerWindow *win = (struct GG_DebuggerWindow *)win_c;
    if(InterlockedCompareExchange(&win->needed_changed, 2, 1) != 1)
        return 0;
    
    out_start[0] = win_c->needed_start;
    out_end[0] = win_c->needed_end;
    
    /* Windows magic */
    win->needed_changed = 0;
    
    return 1;
}
