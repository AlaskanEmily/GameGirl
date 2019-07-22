/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GG_GPU_GFX_H
#define GG_GPU_GFX_H
#pragma once

/* Actually draws to an OS window */
#include "../gg_call.h"

#ifdef __cplusplus
#define GG_GFX_FUNC(T) extern "C" GG_CCALL(T)
#else
#define GG_GFX_FUNC GG_CCALL
#endif

struct GG_Window_s;
typedef struct GG_Window_s GG_Window;
typedef GG_Window *GG_WindowPtr;

GG_GFX_FUNC(void) GG_InitGraphics(void);

GG_GFX_FUNC(GG_WindowPtr) GG_CreateWindow(void);
GG_GFX_FUNC(void) GG_DestroyWindow(GG_Window *win);

GG_GFX_FUNC(void) GG_Flipscreen(GG_Window *win, void *scr);
GG_GFX_FUNC(void) GG_HandleEvents(GG_Window *win, void *scr);

GG_GFX_FUNC(void) GG_BrowseForFile(GG_Window *win,
    const char *ext,
    char *out,
    unsigned out_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* GG_GPU_GFX_H */
