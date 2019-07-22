/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef GG_CALL_H
#define GG_CALL_H
#pragma once

/* Define cdecl */
#if (defined __GNUC__) || (defined __TINYC__)

#ifdef _WIN32
#define GG_CCALL(T) __attribute__((cdecl)) T
#define GG_CCALL_CALLBACK(T, NAME) __attribute__((cdecl))T(*NAME)
#else
#define GG_CCALL(T) T
#define GG_CCALL_CALLBACK(T, NAME) T(*NAME)
#endif

#elif (defined _MSC_VER) || (defined __WATCOMC__)
#define GG_CCALL(T) T __cdecl
#define GG_CCALL_CALLBACK(T, NAME) T(__cdecl*NAME)
#else
#error Add cdecl for your compiler here.
#endif

/* Define stdcall */
#if (defined __GNUC__) || (defined __TINYC__)

#ifdef _WIN32
#define GG_STDCALL(T) __attribute__((stdcall)) T
#define GG_STDCALL_CALLBACK(T, NAME) __attribute__((stdcall))T(*NAME)
#else
#define GG_STDCALL(T) T
#define GG_STDCALL_CALLBACK(T, NAME) T(*NAME)
#endif

#elif (defined _MSC_VER) || (defined __WATCOMC__)
#define GG_STDCALL(T) T __stdcall
#define GG_STDCALL_CALLBACK(T, NAME) T(__stdcall*NAME)
#else
#error Add stdcall for your compiler here.
#endif

#endif /* GG_CALL_H */