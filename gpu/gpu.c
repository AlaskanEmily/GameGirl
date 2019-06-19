/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gpu.h"

#include "blit.h"
#include "gfx.h"
#include "mmu.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Contains actual GPU timing and logic.
 * blit.c/blit.h contains the blit routines and sprite/tile reading.
 */

struct GG_GPU_s {
    unsigned char mode;
    unsigned char line;
    char _1, _2; /* Unused */
    unsigned modeclock;
    
    GG_Screen *screen;
};

const unsigned gg_gpu_struct_size = sizeof(struct GG_GPU_s);
const unsigned _gg_gpu_struct_size = sizeof(struct GG_GPU_s);

void GG_GPU_Init(GG_GPU *gpu){
    memset(gpu, 0, sizeof(GG_GPU));
    gpu->screen = GG_CreateScreen();
}

void GG_GPU_Fini(GG_GPU *gpu){
    GG_DestroyScreen(gpu->screen);
}

unsigned char GG_GPU_GetMode(GG_GPU *gpu){
    return gpu->mode;
}

void GG_GPU_SetMode(GG_GPU *gpu, unsigned char mode){
    gpu->mode = mode;
}

unsigned GG_GPU_GetModeClock(GG_GPU *gpu){
    return gpu->modeclock;
}

void GG_GPU_SetModeClock(GG_GPU *gpu, unsigned modeclock){
    gpu->modeclock = modeclock;
}

static void gg_gpu_flipscreen(GG_GPU *gpu, GG_Window *win){
    GG_Flipscreen(win, gpu->screen);
}

static void gg_gpu_render_line(GG_GPU *gpu, GG_MMU *mmu){
    /* Get the X/Y values */
    const unsigned char lcdcontrol = GG_Read8MMU(mmu, 0xFF40);
    /*
    const unsigned char scrolly = GG_Read8MMU(mmu, 0xFF41);
    const unsigned char scrollx = GG_Read8MMU(mmu, 0xFF43);
    
    const unsigned char backgnd_palette = GG_Read8MMU(mmu, 0xFF47);
    const unsigned char sprite_palette1 = GG_Read8MMU(mmu, 0xFF48);
    const unsigned char sprite_palette2 = GG_Read8MMU(mmu, 0xFF49);
    
    const unsigned char wndy = GG_Read8MMU(mmu, 0xFF4A);
    const unsigned char wndx = GG_Read8MMU(mmu, 0xFF4B);
    */
    /* const unsigned char curline = GG_Read8MMU(mmu, 0xFF44); */
    const unsigned char curline = gpu->line;
    
    register int i;
    
    /* This should have been kept up to date */ /*
    printf("%i == %i\n", curline, GG_Read8MMU(mmu, 0xFF44));
    assert(curline == GG_Read8MMU(mmu, 0xFF44));
    */
    
    if(lcdcontrol != 0)
        printf("ldcontrol=0x%0.2X\n", lcdcontrol);
    
    /* Draw Background. 
     * Bit 0 of LCDCONTROL enables/disables the background
     */
    if(lcdcontrol & 1){
        const unsigned short background_tileset_addr =
            (lcdcontrol & (1<<3)) ? 0x8000 : 0x8800;
        const unsigned short background_map_addr =
            (lcdcontrol & (1<<3)) ? 0x9C00: 0x9800;
        register int x;
        const int tile_y = curline >> 3;
        i = curline & 7;
        /* TODO: Actually do scrolling. This just draws the first
         * 20x18 tiles
         */
        for(x = 0; x < 20; x++){
            /* Read what the tile is */
            const unsigned char pattern_index = GG_Read8MMU(mmu,
                background_map_addr + x + (tile_y << 5));
            
            /* Load and draw the tile */
            const unsigned short tile_line = GG_Read16MMU(mmu,
                background_tileset_addr + (pattern_index << 4) + (i << 1));
            
            GG_BlitLine(gpu->screen, tile_line, x << 3, curline);
        }
    }
    
    /* Draw Sprites */
    for(i = 0; i < 160; i+=4){
        const unsigned char spritex = GG_Read8MMU(mmu, 0xFE00 + i);
        const unsigned char spritey = GG_Read8MMU(mmu, 0xFE01 + i);
        
        if(spritex == 0 && spritey == 0){
            /* Sprite is hidden */
            continue;
        }
        else if(spritey > curline || spritey + 8 <= curline){
            /* Sprite isn't present on this line. */
            continue;
        }
        else{
            const unsigned char pattern_line = curline - spritey;
            const unsigned char pattern_index = GG_Read8MMU(mmu, 0xFE02 + i);
            
            /* Load and draw the sprite */
            const unsigned short sprite_line = GG_Read16MMU(mmu,
                0x8000 + (pattern_index << 4) + (pattern_line << 2));
            
            assert(pattern_line < 8);
            
            /* Draw the sprite line */
            GG_BlitLine(gpu->screen, sprite_line, spritex, curline);
        }
    }
    
    /* Update GPU memory */
    GG_Write8MMU(mmu, 0xFF44, ++(gpu->line));
}

unsigned GG_GPU_Advance(GG_GPU *gpu,
    void *win,
    void *mmu,
    unsigned clock,
    on_gpu_advance_callback cb,
    void *cb_arg){

    assert(mmu != NULL);
    assert(gpu != NULL);
    assert(clock < 0x10000);
    
    {
        const unsigned old_clock = gpu->modeclock;
        unsigned new_clock = old_clock + clock;
        switch(gpu->mode){
            case GG_GPU_HBLANK_MODE: /* 0 */
                if(new_clock >= 816){
                    const unsigned old_line = gpu->line;
                    new_clock -= 816;
                    if(old_line == 144){
                        /* Enter VBLANK. */
                        gpu->mode = GG_GPU_VBLANK_MODE;
                        gg_gpu_flipscreen(gpu, win);
                        gpu->line = 0;
                        if(cb)
                            cb(cb_arg);
                    }
                    else{
                        gpu->mode = GG_GPU_OAM_MODE;
                        gpu->line = old_line + 1;
                    }
                }
                break;
            case GG_GPU_VBLANK_MODE: /* 1 */
                if(new_clock >= 1824){
                    new_clock -= 1824;
                    /* VBLANK */
                    gpu->line++;
                    if(gpu->line >= 144){
                        /* Restart scanline mode */
                        gpu->line = 0;
                        gpu->mode = GG_GPU_OAM_MODE;
                    }
                    GG_Write8MMU(mmu, 0xFF44, gpu->line);
                }
                break;
            case GG_GPU_OAM_MODE: /* 2 */
                if(new_clock >= 320){
                    new_clock -= 320;
                    gpu->mode = GG_GPU_VRAM_MODE;
                }
                break;
            case GG_GPU_VRAM_MODE: /* 3 */
                if(new_clock >= 688){
                    new_clock -= 688;
                    gpu->mode = GG_GPU_HBLANK_MODE;
                    gg_gpu_render_line(gpu, mmu);
                }
                break;
        }
        gpu->modeclock = new_clock;
    }
    
    return gpu->mode;
}
