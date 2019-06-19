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

