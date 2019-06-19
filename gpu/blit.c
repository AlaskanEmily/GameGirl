/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "blit.h"

#include "mmu.h"

#if (defined __unix) && (!defined GG_NO_MMAP)

#include <unistd.h>
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

GG_Screen *GG_CreateScreen(void){
    void *const r = mmap(NULL,
        sizeof(struct GG_Screen_s),
        PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS,
        -1, 0);
    return r;
}

void GG_DestroyScreen(GG_Screen *scr){
    munmap(scr, sizeof(struct GG_Screen_s));
}

#elif (defined WIN32) || (defined _WIN32)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

GG_Screen *GG_CreateScreen(void){
    void *const data = VirtualAlloc(NULL,
        sizeof(struct GG_Screen_s),
        MEM_COMMIT|MEM_RESERVE,
        PAGE_READWRITE);
    
    VirtualLock(data, sizeof(struct GG_Screen_s));
    
    return data;
}
    
void GG_DestroyScreen(GG_Screen *scr){
    VirtualFree(scr, 0, MEM_RELEASE);
}

#else

#include <stdlib.h>

GG_Screen *GG_CreateScreen(void){
    return malloc(sizeof(struct GG_Screen_s));
}

void GG_DestroyScreen(GG_Screen *scr){
    free(scr);
}

#endif

void GG_BlitLine(GG_Screen *const scr,
    const unsigned short pattern,
    const unsigned char x,
    const unsigned char y){
    
    unsigned i;
    unsigned char sprite1 = pattern, sprite2 = (pattern >> 7);
    
    /* Decode the sprite data into color indices */
    for(i = 0; i < 8; i++){
        const unsigned char color = (sprite1 & 1) | (sprite2 & 2);
        /* TODO: PALETTE NOT SET UP */
        
        scr->pixels[x + (y * 160)] = (color << 3) | (color << 8) | (color << 13);
        
        sprite1>>=1;
        sprite2>>=1;
    }
    
}
