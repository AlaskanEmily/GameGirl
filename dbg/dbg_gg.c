/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dbg.h"
#include "mmu.h"
#include "cpu.h"

#include <stdlib.h>
#include <assert.h>

/* The glue that makes the debugger work with the rest of the emulator */

/*****************************************************************************/

struct GG_Debugger{
    GG_CPU *cpu;
    GG_MMU *mmu;
    int state;
    
    /* TODO: This should really be sorted. */
    unsigned num_breaks, cap_breaks;
    unsigned *breaks;
};

/*****************************************************************************/

struct GG_Debugger *GG_CreateDebugger(void *cpu, void *mmu){
    struct GG_Debugger *const dbg = malloc(sizeof(struct GG_Debugger));
    
    assert(dbg);
    assert(mmu);
    assert(cpu);
    
    if(dbg){
        dbg->cpu = cpu;
        dbg->mmu = mmu;
        dbg->breaks = NULL;
        dbg->num_breaks = 0;
        dbg->cap_breaks = 0;
    }
    
    return dbg;
}

/*****************************************************************************/

void GG_DestroyDebugger(struct GG_Debugger *dbg){
    assert(dbg);
    free(dbg);
}

/*****************************************************************************/

int GG_GetDebuggerState(const struct GG_Debugger *dbg){
    assert(dbg);
    assert(dbg->state == GG_DBG_PAUSE || dbg->state == GG_DBG_CONTINUE);
    return dbg->state;
}

/*****************************************************************************/

void GG_SetDebuggerState(struct GG_Debugger *dbg, int state){
    assert(dbg);
    assert(state == GG_DBG_PAUSE || state == GG_DBG_CONTINUE);
    assert(dbg->state == GG_DBG_PAUSE || dbg->state == GG_DBG_CONTINUE);
    
    dbg->state = state;
}

/*****************************************************************************/

void GG_DebuggerSetBreakpoint(struct GG_Debugger *dbg, unsigned address){
    const unsigned num_breaks = dbg->num_breaks;

    unsigned new_cap = 0;
    const unsigned old_cap = dbg->cap_breaks;
    
    assert(old_cap <= num_breaks);
    
    if(old_cap == 0){
        new_cap = 16;
    }
    else if(old_cap == num_breaks){
        new_cap = old_cap << 1;
    }
    if(new_cap != 0){
        unsigned *new_breaks = realloc(dbg->breaks, sizeof(unsigned)*new_cap);
        if(new_breaks == NULL){
            /* DANGER WILL ROBINSON */
            return;
        }
        else{
            dbg->breaks = new_breaks;
        }
    }
    
    
    dbg->breaks[dbg->num_breaks++] = address;
}

/*****************************************************************************/

void GG_DebuggerUnsetBreakpoint(struct GG_Debugger *dbg, unsigned address){
    unsigned i;
    const unsigned num_breaks = dbg->num_breaks;
    
    assert(GG_DebuggerIsBreakpoint(dbg, address));
    
    for(i = 0; i < num_breaks; i++){
        if(dbg->breaks[i] == address){
            /* TODO: This will have to be changed when we start sorting */
            dbg->breaks[i] = dbg->breaks[num_breaks-1];
            dbg->num_breaks = num_breaks-1;
            return;
        }
    }
}

/*****************************************************************************/

void GG_DebuggerUnsetAllBreakpoints(struct GG_Debugger *dbg){
    dbg->num_breaks = 0;
}

/*****************************************************************************/
/* Returns zero for no breakpoint, non-zero if there is a breakpoint */
int GG_DebuggerIsBreakpoint(struct GG_Debugger *dbg, unsigned address){
    unsigned i;
    const unsigned num_breaks = dbg->num_breaks;
    for(i = 0; i < num_breaks; i++){
        if(dbg->breaks[i] == address){
            return 1;
        }
    }
    return 0;
}

/*****************************************************************************/

unsigned GG_DebuggerAddressToLine(const struct GG_Debugger *dbg, unsigned a){
    /* TODO! */
    return 0;
}

/*****************************************************************************/

unsigned GG_DebuggerLineToAddress(const struct GG_Debugger *dbg, unsigned l){
    /* TODO! */
    return 0;
}

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

