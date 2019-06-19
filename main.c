#include "mmu/mmu.h"
#include "cpu/cpu.h"
#include "gpu/gfx.h"
#include "gpu/gpu.h"
#include "dbg/dbg.h"

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

static char buffer[0x400];

int main(int argc, char *argv[]){
    GG_MMU *const mmu = GG_CreateMMU();
    GG_CPU *const cpu = alloca(gg_cpu_struct_size);
    GG_GPU *const gpu = alloca(gg_gpu_struct_size);
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
        rom_name = buffer;
        GG_BrowseForFile(win, ".gb", buffer, sizeof(buffer));
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
    
    }
    
    GG_CPU_Execute(cpu, mmu, gpu, win);
    
    GG_DestroyWindow(win);
    GG_DestroyMMU(mmu);
    GG_GPU_Fini(gpu);
    return 0;
}