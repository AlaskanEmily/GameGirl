#include "dbg.h"

#ifdef __GNUC__
/* Funny how OpenWatcom and TinyC are easier to deal with :) */
#define __STDC_WANT_LIB_EXT2__ 1
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/*****************************************************************************/
/* Contains routines for manipulating the Debug UI elements.
 * Does not include the actual UI components, which live in ui.win32.c,
 * ui.x11.c, or ui.fltk.cpp depending on the platform.
 */

#ifdef _MSC_VER
#define GG_STRDUP _strdup
#else
#define GG_STRDUP strdup
#endif

/*****************************************************************************/

struct GG_DebuggerUILine{
    unsigned short address;
    unsigned short line;
    const char *str;
};

/*****************************************************************************/

#define GG_DEBUGGER_LINE_IS_OWNED(PTR) ((((unsigned long)(PTR)) & 1) == 1)
#define GG_DEBUGGER_LINE_STRING(PTR) \
    (GG_DEBUGGER_LINE_IS_OWNED(PTR) ? \
    (((const char *)(PTR))-1) : (void*)(PTR))

/*****************************************************************************/

struct GG_DebuggerUIBreak{
    unsigned address;
    unsigned short line;
};

struct GG_DebuggerUI{
    unsigned short num_breaks, cap_breaks;
    struct GG_DebuggerUIBreak *breaks;
    
    unsigned short num_lines, cap_lines;
    struct GG_DebuggerUILine *lines;
};

/*****************************************************************************/

struct GG_DebuggerUI *GG_CreateDebuggerUI(){
    return calloc(1, sizeof(struct GG_DebuggerUI));
}

/*****************************************************************************/

static void gg_debugger_destroy_lines(struct GG_DebuggerUILine *lines,
    unsigned num_lines){
    
    unsigned i;
    for(i = 0; i < num_lines; i++){
        const char *const ptr = lines[i].str;
        if(ptr != NULL && GG_DEBUGGER_LINE_IS_OWNED(ptr))
            free((void*)GG_DEBUGGER_LINE_STRING(ptr));
    }
}

void GG_DestroyDebuggerUI(struct GG_DebuggerUI *ui){
    gg_debugger_destroy_lines(ui->lines, ui->num_lines);
    free(ui->lines);
    free(ui->breaks);
    free(ui);
}

/*****************************************************************************/

unsigned GG_GetDebuggerUINumBreaks(const struct GG_DebuggerUI *ui){
    return ui->num_breaks;
}

/*****************************************************************************/

/* Returns zero on success, non-zero on failure */
int GG_SetDebuggerUINumBreaks(struct GG_DebuggerUI *ui, unsigned n){

    assert(ui != NULL);
    assert(ui->cap_breaks <= ui->num_breaks);
    
    {
        unsigned short new_cap;
        const unsigned short old_cap = ui->cap_breaks;
        
        
        if(old_cap == 0){
            new_cap = 16;
        }
        else if(old_cap < n){
            new_cap = old_cap << 1;
            while(new_cap < n){
                new_cap <<= 1;
            }
        }
        else{
            assert(old_cap >= n);
            ui->num_breaks = n;
            return 0;
        }
        
        
        {
            struct GG_DebuggerUIBreak *const new_breaks =
                calloc(new_cap, sizeof(struct GG_DebuggerUIBreak));
            struct GG_DebuggerUIBreak *const old_breaks = ui->breaks;
            const unsigned old_num_breaks = ui->num_breaks;
            if(new_breaks == NULL)
                return 1;
            memcpy(new_breaks, old_breaks,
                old_num_breaks*sizeof(struct GG_DebuggerUIBreak));
            
            free(old_breaks);
            
            if(n > old_num_breaks)
                memset(new_breaks + old_num_breaks, 0, n - old_num_breaks);
            
            ui->breaks = new_breaks;
            ui->num_breaks = n;
        }
        return 0;
    }
}

/*****************************************************************************/

struct GG_DebuggerUIBreak *GG_GetDebuggerUIBreak(struct GG_DebuggerUI *ui,
    unsigned n){
    
    assert(ui != NULL);
    assert(ui->cap_breaks <= ui->num_breaks);
    assert(ui->num_breaks > n);
    assert(ui->breaks != NULL);
    
    return ui->breaks + n;
}

/*****************************************************************************/

const struct GG_DebuggerUIBreak *GG_GetConstDebuggerUIBreak(
    const struct GG_DebuggerUI *ui, unsigned n){
    
    assert(ui != NULL);
    assert(ui->cap_breaks <= ui->num_breaks);
    assert(ui->num_breaks > n);
    assert(ui->breaks != NULL);
    
    return ui->breaks + n;
}

/*****************************************************************************/

void GG_SetDebuggerUIBreak(struct GG_DebuggerUIBreak *br,
    unsigned addr, unsigned line){
    
    assert(br != NULL);
    
    br->address = addr;
    br->line = line;
}

/*****************************************************************************/

unsigned GG_GetDebuggerUIBreakAddress(const struct GG_DebuggerUIBreak *br){
    assert(br != NULL);
    
    return br->address;
}

/*****************************************************************************/

void GG_SetDebuggerUIBreakAddress(struct GG_DebuggerUIBreak *br,
    unsigned addr){
    
    assert(br != NULL);
    
    br->address = addr;
}

/*****************************************************************************/

unsigned GG_GetDebuggerUIBreakLine(const struct GG_DebuggerUIBreak *br){
    assert(br != NULL);
    
    return br->line;
}

/*****************************************************************************/

void GG_SetDebuggerUIBreakLine(struct GG_DebuggerUIBreak *br, unsigned line){
    assert(br != NULL);
    
    br->line = line;
}

/*****************************************************************************/
/* Returns zero on success, non-zero on failure */
int GG_SetDebuggerUINumLines(struct GG_DebuggerUI *ui, unsigned lines){
    
    assert(ui != NULL);
    assert(ui->cap_lines <= ui->num_lines);
    
    if(ui->num_lines > lines){
        const unsigned num_del_lines = ui->num_lines - lines;
        struct GG_DebuggerUILine *const start_del = ui->lines + lines;
        gg_debugger_destroy_lines(start_del, num_del_lines);
        memset(start_del, 0, sizeof(struct GG_DebuggerUILine) * num_del_lines);
        return 0;
    }
    
    {
        unsigned short new_cap;
        const unsigned short old_cap = ui->cap_lines;
        
        ui->num_lines = lines;
        
        if(old_cap == 0){
            new_cap = 0x100;
        }
        else if(old_cap < lines){
            new_cap = old_cap << 1;
            while(new_cap < lines){
                new_cap <<= 1;
            }
        }
        else{
            assert(old_cap >= lines);
            return 0;
        }
        
        assert(new_cap > old_cap);
        
        free(ui->lines);
        ui->lines = calloc(new_cap, sizeof(struct GG_DebuggerUILine));
        if(ui->lines == NULL){
            ui->cap_lines = 0;
            ui->num_lines = 0;
            return 1;
        }
        
        return 0;
    }
}

/*****************************************************************************/

struct GG_DebuggerUILine *GG_GetDebuggerUILine(struct GG_DebuggerUI *ui,
    unsigned line){
    
    assert(ui != NULL);
    assert(ui->cap_lines <= ui->num_lines);
    assert(ui->lines != NULL);
    assert(line <= ui->num_lines);
    
    return ui->lines + line;
}

/*****************************************************************************/

const struct GG_DebuggerUILine *GG_GetConstDebuggerUILine(
    const struct GG_DebuggerUI *ui, unsigned line){
    
    assert(ui != NULL);
    assert(ui->cap_lines <= ui->num_lines);
    assert(ui->lines != NULL);
    assert(line <= ui->num_lines);
    
    return ui->lines + line;
}

/*****************************************************************************/

void GG_SetDebuggerUILineSourceLine(struct GG_DebuggerUILine *line,
    unsigned l){
    
    line->line = l;
}

/*****************************************************************************/

unsigned GG_GetDebuggerUILineSourceLine(const struct GG_DebuggerUILine *line){
    return line->line;
}

/*****************************************************************************/

void GG_SetDebuggerUILineAddress(struct GG_DebuggerUILine *line, unsigned a){

    line->address = a;
}

/*****************************************************************************/

unsigned GG_GetDebuggerUILineAddress(const struct GG_DebuggerUILine *line){
    return line->address;
}

/*****************************************************************************/

void GG_SetDebuggerUILineText(struct GG_DebuggerUILine *line,
    const char *text){
    
    const char *const ptr = line->str;
    
    if(GG_DEBUGGER_LINE_IS_OWNED(text)){
        GG_SetDebuggerUILineTextCopy(line, text);
        return;
    }
    
    if(ptr != NULL && GG_DEBUGGER_LINE_IS_OWNED(ptr))
        free((void*)GG_DEBUGGER_LINE_STRING(ptr));
    
    assert(!GG_DEBUGGER_LINE_IS_OWNED(text));
    line->str = text;
}

/*****************************************************************************/

void GG_SetDebuggerUILineTextCopy(struct GG_DebuggerUILine *line,
    const char *text){
    
    const char *const ptr = line->str;
    
    if(ptr != NULL && GG_DEBUGGER_LINE_IS_OWNED(ptr))
        free((void*)GG_DEBUGGER_LINE_STRING(ptr));
    
    {
        const char *new_text = GG_STRDUP(text);
        assert(!GG_DEBUGGER_LINE_IS_OWNED(new_text));
        line->str = new_text+1;
    }
}

/*****************************************************************************/
/* Returns zero on success, non-zero on failure */
int GG_SetDebuggerUILineTextFormat(struct GG_DebuggerUILine *line,
    const char *fmt, ...){

    const char *text;
    va_list args;
    va_start(args, fmt);

#if (defined __GNUC__)
    vasprintf((char**)&text, fmt, args);
#else
    {
        int len;
    
#if (defined _MSC_VER) && (_MSC_VER >= 1500)
        len = _vscprintf(fmt, args);
#elif (defined _MSC_VER)
        {
            FILE *const f = fopen("nul", "w");
            len = _vfprintf(f, fmt, args);
            fclose(f);
        }
#else
        len = vsnprintf(NULL, 0, fmt, args);
#endif
        va_end(args);
        va_start(args, fmt);
        
        text = malloc(len+1);
        assert(!GG_DEBUGGER_LINE_IS_OWNED(text));

#ifdef _MSC_VER
/* Get out of here with that. */
#pragma warning(suppress : 4996)
#endif
        vsprintf((char*)text, fmt, args);
    }
#endif
    va_end(args);
    
    line->str = text+1;
    
    return 0;
}

/*****************************************************************************/

const char *GG_GetDebuggerUILineText(const struct GG_DebuggerUILine *line){
    
    return GG_DEBUGGER_LINE_STRING(line->str);
}
