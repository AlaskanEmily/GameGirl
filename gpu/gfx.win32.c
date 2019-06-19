#include "gfx.h"

#include "blit.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define GG_GFX_CLASS_NAME (L"GG_GFX")

#define GG_WINDOW_STYLE \
    (WS_OVERLAPPED | \
    WS_CAPTION | \
    WS_SYSMENU | \
    WS_MINIMIZEBOX | \
    WS_THICKFRAME | \
    WS_MAXIMIZEBOX)

#define GG_WINDOW_W ((int)160 << 2)
#define GG_WINDOW_H ((int)144 << 2)

#ifdef __TINYC__

/* TCC lacks a commdlg header. 
 * This is mostly stuff we don't really need.
 */
struct OPENFILENAMEA_s {
    DWORD lStructSize;
    HWND hwndOwner;
    HINSTANCE hInstance;
    LPCSTR lpstrFilter;
    LPSTR lpstrCustomFilter;
    DWORD nMaxCustFilter;
    DWORD nFilterIndex;
    LPSTR lpstrFile;
    DWORD nMaxFile;
    LPSTR lpstrFileTitle;
    DWORD nMaxFileTitle;
    LPCSTR lpstrInitialDir;
    LPCSTR lpstrTitle;
    DWORD Flags;
    WORD nFileOffset;
    WORD nFileExtension;
    LPCSTR lpstrDefExt;
    LPARAM lCustData;
    void* _;
    LPCSTR lpTemplateName;
};

typedef struct OPENFILENAMEA_s OPENFILENAMEA;
BOOL APIENTRY GetOpenFileNameA(OPENFILENAMEA*);

#define GG_OPEN_FILE_FLAGS (0x1800)

#else
#include <commdlg.h>
#define GG_OPEN_FILE_FLAGS (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST)
#endif

struct GG_Window_s{
    HBITMAP bitmap;
    HWND window;
    unsigned scale;
};

static LRESULT CALLBACK gg_gfx_proc(HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam){
    
    struct GG_Window_s *win;
    PAINTSTRUCT ps;
    HDC memDC, winDC;
    
    switch(msg){
        case WM_PAINT:
            {
                win = (void*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
                winDC = BeginPaint(hwnd, &ps);
                memDC = CreateCompatibleDC(winDC);
                
                SetStretchBltMode(hwnd, HALFTONE);
                
                SelectObject(memDC, win->bitmap);
                StretchBlt(winDC, 0, 0, 160 * win->scale, 144 * win->scale,
                    memDC, 0, 0, 160, 144, SRCCOPY);
                EndPaint(hwnd, &ps);
                DeleteDC(memDC);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static BITMAPINFO gg_bmp_info;

void GG_InitGraphics(void){
    const HANDLE cursor = LoadCursor(NULL, IDC_ARROW);
    WNDCLASSW wc = {
        CS_HREDRAW | CS_VREDRAW,
        gg_gfx_proc,
        0,
        0,
        0,
        NULL,
        NULL,
        (HBRUSH)(COLOR_BACKGROUND),
        NULL,
        GG_GFX_CLASS_NAME
    };
    
    wc.hCursor = cursor;
    
    RegisterClassW(&wc);
    
    
    ZeroMemory(&gg_bmp_info, sizeof(BITMAPINFO));
    
    gg_bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    gg_bmp_info.bmiHeader.biWidth = 160;
    gg_bmp_info.bmiHeader.biHeight = 144;
    gg_bmp_info.bmiHeader.biPlanes = 1;
    gg_bmp_info.bmiHeader.biBitCount = 16;
    gg_bmp_info.bmiHeader.biCompression = BI_RGB;
    gg_bmp_info.bmiHeader.biXPelsPerMeter =
        gg_bmp_info.bmiHeader.biYPelsPerMeter = 2835;
    
    /* Just for safety :) */
    assert(GG_OPEN_FILE_FLAGS == (0x1800));
}

GG_Window *GG_CreateWindow(void){
    GG_Window *win;
    const HINSTANCE module = GetModuleHandle(NULL);
    
    win = VirtualAlloc(NULL,
        sizeof(struct GG_Window_s),
        MEM_COMMIT|MEM_RESERVE,
        PAGE_READWRITE);
    
    VirtualLock(win, sizeof(struct GG_Window_s));
    
    /* TODO: Make this configurable */
    win->scale = 2;
    
    win->window = CreateWindowW(
        GG_GFX_CLASS_NAME,
        L"GameGirl",
        GG_WINDOW_STYLE,
        0, 0,
        (int)160 * win->scale, (int)144 * win->scale,
        NULL, NULL,
        module,
        NULL);
    
    SetWindowLongPtrW(win->window, GWLP_USERDATA, (LONG_PTR)win);
    {
        HDC dc = GetDC(win->window);
        win->bitmap = CreateCompatibleBitmap(dc, 160, 144);
        ReleaseDC(win->window, dc);
    }
    ShowWindow(win->window, SW_SHOWNORMAL);
    
    return win;
}

void GG_DestroyWindow(GG_Window *win){
    DestroyWindow(win->window);
    DeleteObject(win->bitmap);
    VirtualFree(win, 0, MEM_RELEASE);
}

void GG_Flipscreen(GG_Window *win, void *scr_v){
    const GG_Screen *const scr = scr_v;
    
    /* Draw the new screen. */
    if(scr != NULL){
        HDC dc = GetDC(win->window);
        SetDIBits(dc, win->bitmap, 0, 144, scr->pixels, &gg_bmp_info, 0);
        ReleaseDC(win->window, dc);
    }
    /* Force a redraw */
    RedrawWindow(win->window, NULL, NULL, RDW_UPDATENOW|RDW_ALLCHILDREN);
    
    /* Handle events. */
    {
        MSG msg;
        while(PeekMessage(&msg, win->window, 0, 0, PM_REMOVE)){
            DispatchMessage(&msg);
        }
    }
}

void GG_BrowseForFile(GG_Window *win,
    const char *ext,
    char *out,
    unsigned out_len){
    
    OPENFILENAMEA info;
    const unsigned ext_len = strlen(ext);
    /* Realloc in order to double-NULL terminate */
    const unsigned new_ext_len = ext_len + 3 + sizeof("File");
    char *const new_ext = malloc(new_ext_len);
    strcpy(new_ext, "File");
    new_ext[sizeof("File")] = '*';
    memcpy(new_ext + sizeof("File") + 1, ext, ext_len + 1);
    new_ext[new_ext_len - 1] = 0;
    ZeroMemory(&info, sizeof(OPENFILENAMEA));
    info.lStructSize = sizeof(OPENFILENAMEA);
    info.hwndOwner = win->window;
    info.lpstrFilter = new_ext;
    info.nFilterIndex = 1;
    info.Flags = GG_OPEN_FILE_FLAGS;
    info.lpstrFile = out;
    info.lpstrFile[0] = 0;
    info.nMaxFile = out_len;
    
    {
        const BOOL ok = GetOpenFileNameA(&info);
        free(new_ext);
        if(!ok)
            out[0] = 0;
        assert(info.lpstrFile == out);
    }
}
