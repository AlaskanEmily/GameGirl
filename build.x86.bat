@echo off

if "%1"=="clean" ( goto clean ) else ( goto build )

:clean

del *.obj 2> nul || type nul
del *.exe 2> nul || type nul
@exit /B 0

:build

if "%1"=="watcom" (
    wmake -e -f makefile.wat
    exit /B %ERRORLEVEL%
) else (
    if "%1"=="hybrid" (
        wmake -e -f makefile.wat cpu.obj
        set "MAKE=wmake.exe"
        set GGSETMAKE=1
    )
    goto buildtcc
)

:buildtcc

set "GGOLDPATH=%PATH%"
set "PATH=%~dp0\tools\tcc;%~dp0\tools\yasm;%PATH%"
set GGOPTIONS=COMPILER=tcc COMPILERFLAGS="-rdynamic -shared -c -Immu -Icpu -Igpu -Idbg -O2 -DNDEBUG" COMPILEOUT="-o " LINKER=tcc LINKOUT="-o "

:findmake


if defined MAKE (
    type nul
) else (
    where wmake 2> nul
    if ERRORLEVEL 1 (
        where nmake 2> nul
        if ERRORLEVEL 1 (
            type nul
        ) else (
            set "MAKE=nmake.exe /nologo"
            set GGSETMAKE=1
        )
    ) else (
        set "MAKE=wmake.exe"
        set GGSETMAKE=1
    )
)

if defined MAKE (
    set "GGMAKE=%MAKE%"
) else (
    set "GGMAKE=%~dp0\tools\bmake\bmake.exe"
)

:callmake

%GGMAKE% "BACKEND=win32" "GFXLIBRARY=-lgdi32 -luser32 -lcomdlg32 -lcomctl32" SOFLAGS="-shared -rdynamic gg.def" ARCH=x86 PLATFORM=elf32 DELETE=del %GGOPTIONS% EXE=.exe SO=.dll OBJ=.obj LIB=.lib %*

if defined GGSETMAKE set MAKE=
set GGSETMAKE=

set GGMAKE=
if defined GGOLDPATH set "PATH=%GGOLDPATH%"
set GGOLDPATH=
set GGOPTIONS=
