#!/bin/sh

make BACKEND=win32 GFXLIBRARY="-lgdi32 -luser32 -lcomdlg32" ARCH=amd64 PLATFORM=elf64 DELETE=rm COMPILER=gcc COMPILERFLAGS="-c -Immu -Icpu -Igpu -Idbg -O2 -Wall -Wextra -pedantic -g -ansi -DNDEBUG" COMPILEOUT="-o " LINKER=gcc LINKFLAGS="-g" LINKOUT="-o " EXE=.exe OBJ=.o LIB=.a $*
