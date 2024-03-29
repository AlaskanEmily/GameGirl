/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dbg_ui.h"
#include "dbg_core.h"

#define __STDC_WANT_LIB_EXT1__ 1
#define WIN32_LEAN_AND_MEAN 1

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#include <Windows.h>
#include <malloc.h>

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

#define GG_OPEN_ROM     1
#define GG_SAVE_DISASM  2
#define GG_SAVE_TILES   3
#define GG_SET_BREAKPOINT 4
#define GG_CLEAR_BREAKPOINT 5
#define GG_CLEAR_ALL_BREAKPOINTS 6

#define GG_SET_REGISTER 0x80

/*****************************************************************************/

#if (defined __TINYC__) || (defined __CYGWIN__)
#define GG_DBG_USE_MUTEX
#endif

#ifdef GG_DBG_USE_MUTEX
struct GG_PendingCommand;
typedef struct GG_PendingCommand *GG_PendingCommandList;
typedef GG_PendingCommandList GG_PendingCommandListHead;
static void *GG_POP_PENDING_CMD(struct GG_PendingCommand **cmd);
#else
#define GG_POP_PENDING_CMD InterlockedPopEntrySList
typedef SLIST_ENTRY GG_PendingCommandList;
typedef SLIST_HEADER GG_PendingCommandListHead;
#endif

/*****************************************************************************/

struct GG_PendingCommand {
    GG_PendingCommandList entry;
    UCHAR type;
    union {
        struct {
            unsigned short address;
        } breakpoint, br;
        struct {
            unsigned short value;
            const char *reg;
        } reg;
    } data;
};

/*****************************************************************************/

#ifdef GG_DBG_USE_MUTEX
static void *GG_POP_PENDING_CMD(struct GG_PendingCommand **cmd){
    struct GG_PendingCommand *const next = *cmd;
    if(next != NULL){
        cmd[0] = next->entry;
    }
    return next;
}
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

struct GG_DBG_UI_s {
#ifdef GG_DBG_USE_MUTEX
    HANDLE pending_cmd_mutex;
#endif
    GG_PendingCommandListHead pending_cmd;
    HWND win;
    GG_DBG *dbg;
    HANDLE signal_event;
    HANDLE thread;
    /* needed_change is a spinlock used to lock needed_start and needed_end */
    volatile LONG needed_changed, was_closed;
    LONG needed_start, needed_end, line_at; /* line_at is set by update */
};

/*****************************************************************************/

const unsigned gg_dbg_ui_struct_size = sizeof(GG_DBG_UI);
const unsigned _gg_dbg_ui_struct_size = sizeof(GG_DBG_UI);

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
#define CW_USEDEFAULT 400
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

static void gg_debugger_command(HWND hwnd, WPARAM wparam, LPARAM lparam){
    GG_DBG_UI *const win = (void*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    assert(win != NULL);
    assert(win->win == hwnd);
    switch((int)wparam){
        case GG_SAVE_DISASM:
            break; /* TODO! */
        case GG_SET_BREAKPOINT:
            break; /* TODO! */
        case GG_CLEAR_BREAKPOINT:
            break; /* TODO! */
        case GG_CLEAR_ALL_BREAKPOINTS:
        {
            struct GG_PendingCommand *const cmd =
                malloc(sizeof(struct GG_PendingCommand));
            cmd->type = (int)wparam;
#ifdef GG_DBG_USE_MUTEX
            WaitForSingleObject(win->pending_cmd_mutex, INFINITE);
            cmd->entry = win->pending_cmd;
            win->pending_cmd = cmd;
            ReleaseMutex(win->pending_cmd_mutex);
#else
            InterlockedPushEntrySList(&win->pending_cmd, &cmd->entry);
#endif
        }
    }
}

/*****************************************************************************/

static LRESULT CALLBACK gg_debugger_window_proc(HWND hwnd,
    UINT msg, WPARAM wparam, LPARAM lparam){
    switch(msg){
        case WM_DESTROY: /* FALLTHROUGH */
        case WM_CLOSE: /* FALLTHROUGH */
        case WM_NCDESTROY:
            PostQuitMessage(0);
            ExitThread(0);
        case WM_COMMAND:
            /* Menu item */
            gg_debugger_command(hwnd, wparam, lparam);
            break;
        case EN_CHANGE:
            break;
        case EN_KILLFOCUS:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/*****************************************************************************/

#define GG_NUM_COLUMNS 4

#ifdef __TINYC__

#define GG_STATIC_LVCOL_EXTRA(WIDTH)

#ifndef LVCF_IDEALWIDTH
#define LVCF_IDEALWIDTH 0
#endif

#else

#define GG_STATIC_LVCOL_EXTRA(WIDTH) , 0, 0, (WIDTH)

#endif

#define GG_STATIC_LVCOL(NAME, ALIGN, WIDTH, SUBITEM) { \
    LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_IDEALWIDTH, \
    (ALIGN), \
    (WIDTH), \
    (NAME), \
    0, \
    (SUBITEM), \
    0 \
    GG_STATIC_LVCOL_EXTRA(WIDTH)}

/*****************************************************************************/

static const LVCOLUMNW gg_register_col0 =
    GG_STATIC_LVCOL(L"Reg", LVCFMT_RIGHT, 40, 0);

static const LVCOLUMNW gg_register_col1 = 
    GG_STATIC_LVCOL(L"Val", LVCFMT_LEFT, 40, 1);

/*****************************************************************************/

static const LVCOLUMNW gg_list_view_col0 =
    GG_STATIC_LVCOL(L"Address", LVCFMT_LEFT, 80, 0);

static const LVCOLUMNW gg_list_view_col1 =
    GG_STATIC_LVCOL(L"Break", LVCFMT_LEFT, 40, 1);

static const LVCOLUMNW gg_list_view_col2 =
    GG_STATIC_LVCOL(L"Assembly code", LVCFMT_LEFT, 100, 2);

static const LVCOLUMNW gg_list_view_col3 =
    GG_STATIC_LVCOL(L"Assembly code", LVCFMT_LEFT, 80, 3);

/*****************************************************************************/

static DWORD WINAPI gg_debugger_window_thread_proc(void *param){
    
    GG_DBG_UI *const win = param;
    MSG msg;
    const HINSTANCE instance = GetModuleHandle(NULL);
    
#define GG_ITEM_TEXT_BUFFER_LEN 0x400
    wchar_t items_text_buffer[GG_ITEM_TEXT_BUFFER_LEN];
    HWND list_view, register_view, text_input;
    HMENU menu_bar;
    
    {
        /* Create the menu bar */
        menu_bar = CreateMenu();
        {
            HMENU file_menu = CreateMenu();
            AppendMenuW(file_menu, MF_STRING|MF_GRAYED, GG_OPEN_ROM,
                L"Open rom...");
            
            AppendMenuW(file_menu, MF_STRING,           GG_SAVE_DISASM,
                L"Save disassembly...");
            
            AppendMenuW(file_menu, MF_STRING|MF_GRAYED, GG_SAVE_TILES,
                L"Save tiles...");
            
            AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)file_menu,
                L"File");
        }
        {
            HMENU breakpoint_menu = CreateMenu();
            AppendMenuW(breakpoint_menu, MF_STRING, GG_CLEAR_ALL_BREAKPOINTS,
                L"Clear all breakpoints");
            
            AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)breakpoint_menu,
                L"Breakpoints");
        }
    }
    
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
        menu_bar,
        instance,
        NULL);
    
    SetWindowLongPtrW(win->win, GWLP_USERDATA, (LONG_PTR)win);
    
    win->needed_start = 16;
    win->needed_end = 116;
    win->needed_changed = 1;
    win->was_closed = 0;
    
    /* Report success. */
    if(!SetEvent(win->signal_event)){
		OutputDebugString(TEXT("Could not signal event!"));
		return 0;
	}
    
    /* Create the UI elements */
    
    {
        /* Create the listview used for the disassembly and graphical
         * breakpoint system.
         */
        list_view = CreateWindowExW(WS_EX_CLIENTEDGE,
            WC_LISTVIEWW,
            L"GG_DebuggerListView",
            WS_VISIBLE | WS_CHILD | WS_HSCROLL | LVS_REPORT,
            10,
            10,
            400,
            290,
            win->win,
            NULL,
            instance,
            NULL);
        
        PostMessage(list_view, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
            LVS_EX_FULLROWSELECT);
        
        /* Add the address column */
        PostMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)0,
            (LPARAM)&gg_list_view_col0);
        /* Add the breakpoint column */
        PostMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)1,
            (LPARAM)&gg_list_view_col1);
        /* Add the assembly code column */
        PostMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)2,
            (LPARAM)&gg_list_view_col2);
        /* Add the machine code column */
        PostMessageW(list_view, LVM_INSERTCOLUMNW, (WPARAM)3,
            (LPARAM)&gg_list_view_col3);
    }
    
    {
        /* Create the listview used for the register display */
        const char *const *const reg_name = gg_dbg_register_names;
        register int i = 0;
        
        while(reg_name[i] != NULL){
            
            HWND reg_label, reg_value;
            
            MultiByteToWideChar(CP_UTF8, 0,
                reg_name[i], -1,
                items_text_buffer, GG_ITEM_TEXT_BUFFER_LEN);
            
            /* Intentionally NOT with client edge. */
            reg_label = CreateWindowW(L"Edit", 
                items_text_buffer,
                WS_VISIBLE | WS_CHILD | ES_READONLY,
                430,
                10 + (i*20),
                40,
                20,
                win->win,
                NULL,
                instance,
                NULL);
            /* Intentionally NOT with client edge. */
            reg_label = CreateWindowW(L"EDIT", 
                NULL,
                WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                470,
                10 + (i*20),
                40,
                20,
                win->win,
                (HMENU)(GG_SET_REGISTER+i),
                instance,
                NULL);
            i++;
        }
    }
    
    /* Create the debugger text input box */
    text_input = CreateWindowExW(WS_EX_CLIENTEDGE,
        L"Edit", 
        L"",
        WS_VISIBLE | WS_CHILD,
        10,
        310,
        400,
        20,
        win->win,
        NULL,
        instance,
        NULL);
    SendMessage(text_input, ES_READONLY, FALSE, 0);
    
    ShowWindowAsync(win->win, SW_SHOWNORMAL);
    
    while(GetMessage(&msg, win->win, 0, 0)){
        
        TranslateMessage(&msg);
        /* Check if we've been signalled to regenerate the list */
        if(msg.message == gg_custom_message){
            register ULONG i;
            const LONG needed_start = win->needed_start;
            ULONG num_lines = win->needed_end - needed_start;
            /* const struct GG_DebuggerUI *const ui = win->dbg; */
            
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
                sizeof(gg_column_indices) / sizeof(*gg_column_indices),
                NULL /* columns (TO SET) */
#ifndef __TINYC__
              , NULL /* column format */
              , 0 /* group */
#endif
            };
            item.puColumns = gg_column_indices;
            item.pszText = items_text_buffer;
            
            assert(win->needed_end >= win->needed_start);
            /* Remove all items. We could do an incremental update, but bleh.
             * This is synchronous (SendMessage versus PostMessage) because
             * we are about to clear all the text data.
             */
            SendMessage(list_view, LVM_DELETEALLITEMS, 0, 0);
            
            /* Add all the lines from the debugger UI */
            for(i = 0; i < num_lines; i++){
#if 0
                const struct GG_DebuggerUILine *const line =
                    GG_GetConstDebuggerUILine(ui, i);
                
                /* Reset the subitem. */
                item.iSubItem = 0;
                item.iItem = i;
                {
                    /* Send the address, posting the initial item info. */
                    const unsigned address = GG_GetDebuggerUILineAddress(line);
                    WPRINT_ADDRESS(item.pszText, address);
                    SendMessageW(list_view, LVM_INSERTITEMW, 0, (LPARAM)&item);
                }
                
                /* TODO! IMAGE ON BREAK COLUMN */
                
                /* Remove the columns info. */
                item.mask = LVIF_TEXT;
                {
                    const char *const line_text =
                        GG_GetDebuggerUILineText(line);
                    MultiByteToWideChar(CP_UTF8, 0,
                        line_text, -1,
                        item.pszText, GG_ITEM_TEXT_BUFFER_LEN);
                    item.iSubItem = 2;
                    SendMessageW(list_view, LVM_SETITEMW, 0, (LPARAM)&item);
                }
                {
                    /* Post a mostly-phony raw view (NOT YET IMPLEMENTED!) */
                    item.pszText = L"0x00";
                    item.iSubItem = 3;
                    SendMessageW(list_view, LVM_SETITEMW, 0, (LPARAM)&item);
                }
#endif
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

static BOOL gg_wait_for_debugger(const GG_DBG_UI *win){
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
    
    /* Register the debugger window class */
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
    
    /* Register our custom message type */
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

void GG_DBG_UI_Init(GG_DBG_UI *win, GG_DBG *dbg){
    
    ZeroMemory(win, sizeof(GG_DBG_UI));
#ifdef GG_DBG_USE_MUTEX
    win->pending_cmd_mutex = CreateMutexA(NULL, FALSE, NULL);
    win->pending_cmd = NULL;
#else
    InitializeSListHead(&win->pending_cmd);
#endif
    
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

void GG_DBG_UI_Fini(GG_DBG_UI *win){
    DestroyWindow(win->win);
}

/*****************************************************************************/
/* Updates the debugger window to contain current values from the DebuggerUI.
 * This the only point where the window will read from the UI component.
 */
void GG_DBG_UI_Update(GG_DBG_UI *win){
    SendMessage(win->win, gg_custom_message, 0, 0);
}

/*****************************************************************************/
/* Process any pending events from the debugger window.
 */
int GG_DBG_UI_HandleEvents(GG_DBG_UI *win){
    
    void *e;
    
    /* Nothing to do since we are multithreaded. */
    DWORD result;
    if((!GetExitCodeThread(win->thread, &result)) || result != STILL_ACTIVE){
        CloseHandle(win->thread);
        return 1;
    }
    else{
        /* Process any pending commands */
#ifdef GG_DBG_USE_MUTEX
        /* Wait with just 1. We're OK with just waiting for another frame. */
        if(WaitForSingleObject(win->pending_cmd_mutex, 1) != WAIT_OBJECT_0){
            printf("WaitForSingleObject: %i\n", GetLastError());
            return 0;
        }
#endif
        if((e = GG_POP_PENDING_CMD(&win->pending_cmd)) != NULL){
            do{
                struct GG_PendingCommand *cmd =
                    (struct GG_PendingCommand*)e;
                if(win->dbg != NULL){
                    switch(cmd->type){
                        case GG_SET_BREAKPOINT:
                            GG_DBG_SetBreakpoint(win->dbg, cmd->data.br.address);
                            break;
                        case GG_CLEAR_BREAKPOINT:
                            GG_DBG_UnsetBreakpoint(win->dbg, cmd->data.br.address);
                            break;
                        case GG_CLEAR_ALL_BREAKPOINTS:
                            GG_DBG_UnsetAllBreakpoints(win->dbg);
                            break;
                    }
                }
                free(e);
            }while((e = GG_POP_PENDING_CMD(&win->pending_cmd)) != NULL);
            
            /* TODO: this might not be important? We could return differently
             * instead to avoid the double-post
             */
            SendMessage(win->win, gg_custom_message, 0, 0);
        }
#ifdef GG_DBG_USE_MUTEX
        ReleaseMutex(win->pending_cmd_mutex);
#endif
        return 0;
    }
}

/*****************************************************************************/

int GG_DBG_UI_NeededLines(const GG_DBG_UI *win_c,
    unsigned *out_start,
    unsigned *out_end){
    
    /* Remove the constness...very evil, but needed for the spinlock. */
    GG_DBG_UI *win = (GG_DBG_UI*)win_c;
    if(InterlockedCompareExchange(&win->needed_changed, 2, 1) != 1)
        return 0;
    
    out_start[0] = win_c->needed_start;
    out_end[0] = win_c->needed_end;
    
    /* Windows magic */
    win->needed_changed = 0;
    
    return 1;
}
