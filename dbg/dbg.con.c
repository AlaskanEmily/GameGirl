#include "dbg.h"

/* Slightly evil :) */

#if ((defined _WIN32) || (defined WIN32)) && (!defined __CYGWIN__)

#include <Windows.h>
#include <io.h>

#ifdef __WATCOMC__
#include <stdint.h>
#endif

typedef HANDLE gg_dbg_native_file;
#define GG_GET_NATIVE_HANDLE(F) (HANDLE)_get_osfhandle(_fileno(F))
#define GG_CLOSE_NATIVE_HANDLE(F) _close((intptr_t)(F))
#define GG_NATIVE_HANDLE_CHECK(F) \
    (WaitForSingleObject((F), 0)==WAIT_OBJECT_0)

#else

#ifdef __CYGWIN__
#define _POSIX_SOURCE
#endif

#include <sys/select.h>
#include <unistd.h>
#include <time.h>

typedef int gg_dbg_native_file;
#define GG_GET_NATIVE_HANDLE fileno
#define GG_CLOSE_NATIVE_HANDLE(F) ((void)sizeof(F))
#define GG_NATIVE_HANDLE_CHECK gg_native_handle_check

static int gg_native_handle_check(int f){
    fd_set s;
#ifdef __CYGWIN__
    struct timeval ts;
    ts.tv_sec = 0;
    ts.tv_usec = 0;
#else
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
#endif
    FD_ZERO(&s);
    FD_SET(f, &s);
    return select(f+1, &s, NULL, NULL, &ts) == 1;
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

struct GG_DebuggerWindow{
    struct GG_DebuggerUI *dbg;
    gg_dbg_native_file input_native;
    int active_line, last_active_line;
    FILE *out, *in;
    unsigned read_buffer_size;
    char *read_buffer;
};

/*****************************************************************************/

struct GG_DebuggerWindow* GG_CreateDebuggerWindow(struct GG_DebuggerUI *dbg){
    struct GG_DebuggerWindow *const win =
        malloc(sizeof(struct GG_DebuggerWindow));
    win->dbg = dbg;
    win->active_line = win->last_active_line = 0;
    win->out = stdout;
    win->in = stdin;
    win->input_native = GG_GET_NATIVE_HANDLE(win->in);
    win->read_buffer_size = 0;
    win->read_buffer = NULL;
    
    return win;
}

/*****************************************************************************/

void GG_DestroyDebuggerWindow(struct GG_DebuggerWindow *win){
    if(win->out != stdout)
        fclose(win->out);
    if(win->in != stdin)
        fclose(win->in);
    GG_CLOSE_NATIVE_HANDLE(win->input_native);
    free(win->read_buffer);
    free(win);
}

/*****************************************************************************/
/* Updates the debugger window to contain current values from the DebuggerUI.
 * This the only point where the window will read from the UI component.
 */
void GG_UpdateDebuggerWindow(struct GG_DebuggerWindow *win){
    /* Cheap hack :) */
    fputs("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",
        win->out);
}

/*****************************************************************************/
/* Process any pending events from the debugger window.
 */
void GG_PollDebuggerWindow(struct GG_DebuggerWindow *win,
    struct GG_Debugger *dbg){
    
    unsigned i, address;
    char c;
    
    if(!GG_NATIVE_HANDLE_CHECK(win->input_native))
        return;
    
    if(win->read_buffer_size == 0){
        win->read_buffer_size = 80;
        win->read_buffer = malloc(80);
    }
    
    /* Read from the input */
    i = 0;
    while((!feof(win->in)) && (c = fgetc(win->in)) != '\n'){
        if(i == 0 && isspace(c))
            continue;
            
        /* Realloc our buffer if needed. */
        if(i+1 >= win->read_buffer_size){
            const int new_buffer_size = win->read_buffer_size+80;
            char *const new_buffer =
                realloc(win->read_buffer, new_buffer_size);
            
            if(new_buffer == NULL){
                fputs("OUT OF MEMORY\n", stderr);
                return; /* TODO! Report error! */
            }
            
            win->read_buffer = new_buffer;
            win->read_buffer_size = new_buffer_size;
        }
        win->read_buffer[i++] = c;
    }
    /* NULL-term for sscanf */
    win->read_buffer[i] = '0';
    
    if(win->read_buffer[0] == 'c'){
        GG_SetDebuggerState(dbg, GG_DBG_CONTINUE);
    }
    else if(sscanf(win->read_buffer, " b %u", &address) == 1){
        i = GG_GetDebuggerUINumBreaks(win->dbg);
        if(GG_SetDebuggerUINumBreaks(win->dbg, i+1) == 0){
            const unsigned line = GG_DebuggerAddressToLine(dbg, address);
            struct GG_DebuggerUIBreak *const br = GG_GetDebuggerUIBreak(win->dbg, i);
            GG_SetDebuggerUIBreakAddress(br, address);
            GG_SetDebuggerUIBreakLine(br, line);
        }
        else{
            fputs("Could not create breakpoint\n", stderr);
        }
    }
}

/*****************************************************************************/
/* This must be called after calling GG_PollDebuggerWindow, but before
 * GG_UpdateDebuggerWindow. The caller (usually the debugger core?) must
 * fill out the debugger UI with the disassembly for these lines.
 *
 * Returns non-zero if there are any changes (hopefully there usually aren't?)
 */
int GG_GetDebuggerWindowNeededLines(const struct GG_DebuggerWindow *win,
    unsigned *out_start, unsigned *out_end){
    
    const int active_line = win->active_line;
    const int last_active_line = win->last_active_line;
    
    if(active_line < 8){
        if(last_active_line < 8)
            return 0;
        
        out_start[0] = 0;
        out_end[0] = 16;
    }
    else{
        if(last_active_line == active_line)
            return 0;
        
        out_start[0] = active_line - 8;
        out_end[0] = active_line + 8;
    }
    
    return 1;
}
