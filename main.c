/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mmu/mmu.h"
#include "cpu/cpu.h"
#include "gpu/gfx.h"
#include "gpu/gpu.h"
#include "dbg/dbg.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>

#if (defined _WIN32) || (defined WIN32) || (defined __CYGWIN__)
#include "bufferfile_win32.c"
#else
#include "bufferfile_unix.c"
#endif

/* Get alloca */
#if (defined _MSC_VER) || (defined __WATCOMC__)
#include <malloc.h>
#elif (defined __TINYC__)
#include <stddef.h>
#else
#include <alloca.h>
#endif

static char rom_name_buffer[0x400];

struct debugger_callback_arg{
    struct GG_Debugger *dbg;
    struct GG_DebuggerUI *dbg_ui;
    struct GG_DebuggerWindow *dbg_win;
};

static GG_GPU_FUNC(void) debugger_callback(void *arg_v){
    struct debugger_callback_arg *const arg = arg_v;
    assert(arg);
}

int main(int argc, char *argv[]){
    GG_MMU *const mmu = GG_CreateMMU();
    GG_CPU *const cpu = alloca(gg_cpu_struct_size);
    GG_GPU *const gpu = alloca(gg_gpu_struct_size);
    on_gpu_advance_callback cb = NULL;
    struct debugger_callback_arg debugger_data = {NULL, NULL, NULL};
    GG_Window *win;
    const char *rom_name = NULL;
    const void *rom;
    int rom_size;
    int i;
    /* TODO: This should be changed */
    int start_debugger = 1;
    
    GG_InitGraphics();
    
    /* Create and show the window */
    win = GG_CreateWindow();
    SwitchToThread();
    GG_Flipscreen(win, NULL);
    SwitchToThread();
    
    
    /* Get the rom name. */
    
    for(i = 1; i < argc; i++){
        const char *const arg = argv[i];
        if(arg[0] == '-'){
            int str_i = 1;
            char c;
            if(arg[1] == 0){
                puts("Empty option");
                return 1;
            }
            
            while((c = arg[str_i++]) != 0){
                switch(c){
                    case 'd':
                        start_debugger = 1;
                        break;
                    /* LOLOLOL no options implemented */
                    default:
                        printf("Unknown option %c\n", c);
                        return 1;
                }
            }
        }
        else{
            if(rom_name != NULL){
                puts("Too many rom paths");
                return 1;
            }
            rom_name = arg;
        }
    }
    
    if(rom_name == NULL){
        rom_name = rom_name_buffer;
        GG_BrowseForFile(win, ".gb", rom_name_buffer, sizeof(rom_name_buffer));
    }
    
    /* Load the rom */
    rom = BufferFile(rom_name, &rom_size);
    if(rom == NULL){
        printf("Could not open rom %s\n", rom_name);
        return 1;
    }
    else{
        printf("Opening rom %s\n", rom_name);
    }
    
    GG_SetMMURom(mmu, rom, rom_size);
    
    GG_CPU_Init(cpu, mmu);
    GG_GPU_Init(gpu);
    
    if(start_debugger){
        GG_InitDebuggerWindowSystem();
        debugger_data.dbg = GG_CreateDebugger(cpu, mmu);
        debugger_data.dbg_ui = GG_CreateDebuggerUI();
        debugger_data.dbg_win = GG_CreateDebuggerWindow(debugger_data.dbg_ui);
        cb = debugger_callback;
    }
    
    GG_CPU_Execute(cpu, mmu, gpu, win, debugger_data.dbg, cb, &debugger_data);
    
    GG_DestroyWindow(win);
    GG_DestroyMMU(mmu);
    GG_GPU_Fini(gpu);
    return 0;
}
