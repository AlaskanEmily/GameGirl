#!/bin/sh

# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

make BACKEND=win32 GFXLIBRARY="-lgdi32 -luser32 -lcomdlg32" ARCH=amd64 PLATFORM=elf64 DELETE=rm COMPILER=gcc COMPILERFLAGS="-c -Immu -Icpu -Igpu -Idbg -O2 -Wall -Wextra -pedantic -g -ansi -DNDEBUG" COMPILEOUT="-o " LINKER=gcc LINKFLAGS="-g" LINKOUT="-o " EXE=.exe OBJ=.o LIB=.a $*
