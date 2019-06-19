/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "dbg.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv){

    GG_InitDebuggerWindowSystem();
    
    {
        struct GG_DebuggerUI *const dbg_ui = GG_CreateDebuggerUI();
        struct GG_DebuggerWindow *const dbg_win = GG_CreateDebuggerWindow(dbg_ui);
        while(GG_PollDebuggerWindow(dbg_win) == 0){
            unsigned start, end;
            if(GG_GetDebuggerWindowNeededLines(dbg_win, &start, &end)){
                /* Fill with gibberish. */
                unsigned i;
                assert(end >= start);
                GG_SetDebuggerUINumLines(dbg_ui, end-start);
                for(i = 0; i < end-start; i++){
                    struct GG_DebuggerUILine *const line =
                        GG_GetDebuggerUILine(dbg_ui, i);
                    GG_SetDebuggerUILineTextFormat(line, "Line %i", i + start);
                    /* Just dummy all the addresses out to 4*line */
                    GG_SetDebuggerUILineAddress(line, (i + start) << 2);
                }
                GG_UpdateDebuggerWindow(dbg_win);
            }
        }
        
        return 0;
    }
}
