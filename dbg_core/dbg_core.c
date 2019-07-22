/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dbg_core.h"

#include "mmu.h"
#include "cpu.h"

#include <stdlib.h>
#include <assert.h>

/*****************************************************************************/

struct GG_DBG_s {
    unsigned char state;
    
    GG_CPU *cpu;
    GG_MMU *mmu;
    
    /* TODO: This should really be sorted. */
    unsigned num_breaks, cap_breaks;
    unsigned *breaks;
};

const unsigned gg_dbg_core_struct_size = sizeof(GG_DBG);
const unsigned _gg_dbg_core_struct_size = sizeof(GG_DBG);

/*****************************************************************************/

#define GG_DBG_REGISTER_NAME(X) "" #X "",

/*****************************************************************************/

static const char *const register_names[] = {
    GG_ALL_REGISTERS(GG_DBG_REGISTER_NAME)
    NULL
};

/*****************************************************************************/

const char *const *const gg_dbg_register_names = register_names;
const char *const *const _gg_dbg_register_names = register_names;

/*****************************************************************************/

void GG_DBG_Init(GG_DBG *dbg, void *cpu, void *mmu){
    assert(dbg);
    assert(mmu);
    assert(cpu);
    
    dbg->cpu = cpu;
    dbg->mmu = mmu;
    dbg->breaks = NULL;
    dbg->num_breaks = 0;
    dbg->cap_breaks = 0;
}

/*****************************************************************************/

void GG_DBG_Fini(GG_DBG *dbg){
    free(dbg->breaks);
}

/*****************************************************************************/

void GG_DBG_SetState(GG_DBG *dbg, int state) {
    dbg->state = state;
}

/*****************************************************************************/

int GG_DBG_GetState(const GG_DBG *dbg) {
    return dbg->state;
}


/*****************************************************************************/

void GG_DBG_SetBreakpoint(GG_DBG *dbg, unsigned address){
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

void GG_DBG_UnsetBreakpoint(GG_DBG *dbg, unsigned address){
    unsigned i;
    const unsigned num_breaks = dbg->num_breaks;
    
    assert(GG_DBG_IsBreakpoint(dbg, address));
    
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

void GG_DBG_UnsetAllBreakpoints(GG_DBG *dbg){
    dbg->num_breaks = 0;
}

/*****************************************************************************/
/* Returns zero for no breakpoint, non-zero if there is a breakpoint */
int GG_DBG_IsBreakpoint(GG_DBG *dbg, unsigned address){
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

unsigned GG_DBG_AddressToLine(const GG_DBG *dbg, unsigned a){
    /* TODO! */
    return a;
}

/*****************************************************************************/

unsigned GG_DBG_LineToAddress(const GG_DBG *dbg, unsigned l){
    /* TODO! */
    return l;
}
