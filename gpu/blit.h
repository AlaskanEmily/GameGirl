/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_GPU_BLIT_H
#define GG_GPU_BLIT_H
#pragma once

/* Contains blit routines and sprite/tile reading.
 * gpu.c/gpu.h has the GPU logic.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct GG_Screen_s {
    /* Stored as R5G6B5. */
    unsigned short pixels[160 * 144];
};

typedef struct GG_Screen_s GG_Screen;

GG_Screen *GG_CreateScreen(void);
void GG_DestroyScreen(GG_Screen *);

void GG_BlitLine(GG_Screen *scr,
    unsigned short pattern_data,
    unsigned char x,
    unsigned char y);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* GG_GPU_BLIT_H */
