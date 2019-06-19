# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

all: gg.exe gg_disasm.exe gg_dbg_test.exe

# TODO: Swap bc to be bg?
WCCFLAGS=-ox -zw -bc -br -6r -we -wx -hd -ri -i=cpu -i=mmu -i=gpu -i=dbg -dWIN32=1 -q $(WCCEXTRAFLAGS)
WLINKFLAGS=op map SYS nt op quiet
PROGRAM=gg.exe
DISASM_PROGRAM=gg_disasm.exe
OBJECTS=main.obj mmu.obj cpu.obj dbg_disasm.obj dbg_gg.obj dbg_ui.obj dbg.win32.obj gpu.obj blit.obj gfx.win32.obj
DISASM_OBJECTS=mmu.obj dbg_disasm.obj disasm.obj
DBG_TEST_OBJECTS=dbg_ui.obj dbg.win32.obj dbg_test.obj

hybrid: main.obj cpu.obj gg.dll gg.def
	wlink $(WLINKFLAGS) FILE { main.obj cpu.obj } LIBRARY gg.lib NAME gg.exe
	type nul > hybrid

cpu.obj: cpu\cpu.c cpu\cpu.h cpu\cpu_dummy.h cpu\cpu.inc mmu\mmu.h
	wcc386 cpu\cpu.c $(WCCFLAGS)

dbg_ui.obj: dbg\dbg_ui.c dbg\dbg.h
	wcc386 dbg\dbg_ui.c $(WCCFLAGS)

dbg_gg.obj: dbg\dbg_gg.c dbg\dbg.h cpu\cpu.h mmu\mmu.h
	wcc386 dbg\dbg_gg.c $(WCCFLAGS)

dbg.win32.obj: dbg\dbg.win32.c dbg\dbg.h
	wcc386 dbg\dbg.win32.c $(WCCFLAGS)

dbg_disasm.obj: dbg\dbg_disasm.c dbg\dbg.h cpu\cpu.inc cpu\cpu_dummy.h
	wcc386 dbg\dbg_disasm.c $(WCCFLAGS)

mmu.obj: mmu\mmu.c mmu\mmu.h
	wcc386 mmu\mmu.c $(WCCFLAGS)

gpu.obj: gpu\gpu.c gpu\gpu.h mmu\mmu.h gpu\blit.h
	wcc386 gpu\gpu.c $(WCCFLAGS)

blit.obj: gpu\blit.c gpu\blit.h
	wcc386 gpu\blit.c $(WCCFLAGS)

gfx.gdiplus.obj: gpu\gfx.gdiplus.cpp gpu\gfx.h gpu\blit.h
	wpp386 gpu\gfx.gdiplus.cpp $(WCCFLAGS) -zv -zw -xdt

gfx.win32.obj: gpu\gfx.win32.c gpu\gfx.h gpu\blit.h
	wcc386 gpu\gfx.win32.c $(WCCFLAGS)

main.obj: main.c mmu\mmu.h cpu\cpu.h gpu\gfx.h gpu\gpu.h
	wcc386 main.c $(WCCFLAGS)

disasm.obj: disasm.c mmu\mmu.h dbg\dbg.h
	wcc386 disasm.c $(WCCFLAGS)

dbg_test.obj: dbg_test.c dbg\dbg.h
	wcc386 dbg_test.c $(WCCFLAGS)

gg.exe: $(OBJECTS)
	wlink $(WLINKFLAGS) FILE { $(OBJECTS) } NAME gg.exe

gg_disasm.exe: $(DISASM_OBJECTS)
	wlink $(WLINKFLAGS) FILE { $(DISASM_OBJECTS) } NAME gg_disasm.exe

gg_dbg_test.exe: $(DBG_TEST_OBJECTS)
	wlink $(WLINKFLAGS) FILE { $(DBG_TEST_OBJECTS) } NAME gg_dbg_test.exe
