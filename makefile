# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

YASMFLAGS=-m $(ARCH) -f $(PLATFORM)

PROGRAM=gg$(EXE)
LIBRARY=gg$(SO)
DISASM_PROGRAM=gg_disasm$(EXE)
DBG_TEST_PROGRAM=gg_dbg_test$(EXE)
LIBRARY_OBJECTS=mmu$(OBJ) dbg_disasm$(OBJ) dbg_core$(OBJ) dbg_ui.$(BACKEND)$(OBJ) gpu$(OBJ) blit$(OBJ) gfx.$(BACKEND)$(OBJ) cpu_length$(OBJ) cpu_timings$(OBJ)
CPU_OBJECTS=cpu_timings$(OBJ) cpu_length$(OBJ) cpu$(OBJ)
GPU_OBJECTS=gpu$(OBJ) blit$(OBJ) gfx.$(BACKEND)$(OBJ) 
DBG_OBJECTS=dbg_disasm$(OBJ) dbg_core$(OBJ) dbg_ui.$(BACKEND)$(OBJ)
OBJECTS=main$(OBJ) mmu$(OBJ) $(CPU_OBJECTS) $(GPU_OBJECTS) $(DBG_OBJECTS)
DISASM_OBJECTS=mmu$(OBJ) disasm$(OBJ) cpu_timings$(OBJ) cpu_length$(OBJ) dbg_disasm$(OBJ)
DBG_TEST_OBJECTS=dbg_test$(OBJ) mmu$(OBJ) $(DBG_OBJECTS)

all: $(PROGRAM) $(DISASM_PROGRAM) $(DBG_TEST_PROGRAM)

# Hack for the hybrid build.
# 1. Create gg.lib for gg.dll
# 2. Use Watcom's lib clone (or Microsoft's lib) to make an import library.
# Have wmake complete the build with the hybrid target
hybrid: $(LIBRARY)
	echo > gg.lib
	del gg.lib
	tiny_impdef gg.dll
	wlib -iro -inn gg.lib +gg.dll
	wmake /f makefile.wat hybrid

# cpu$(OBJ): cpu/cpu.$(ARCH).s cpu/cpu.inc cpu/mmu.inc
# 	yasm $(YASMFLAGS) cpu/cpu.$(ARCH).s -o cpu$(OBJ)

cpu$(OBJ): cpu/cpu.c cpu/cpu.h cpu/cpu_dummy.h cpu/cpu.inc mmu/mmu.h gpu/gpu.h
	$(COMPILER) $(COMPILERFLAGS) -c cpu/cpu.c -o cpu$(OBJ)

cpu_length$(OBJ): cpu/cpu_length.c cpu/cpu.inc
	$(COMPILER) $(COMPILERFLAGS) -c cpu/cpu_length.c -o cpu_length$(OBJ)

cpu_timings$(OBJ): cpu/cpu_timings.c cpu/cpu.inc
	$(COMPILER) $(COMPILERFLAGS) -c cpu/cpu_timings.c -o cpu_timings$(OBJ)

dbg_core$(OBJ): dbg_core/dbg_core.c dbg_core/dbg_core.h cpu/cpu.h mmu/mmu.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg_core/dbg_core.c -o dbg_core$(OBJ)

dbg_disasm$(OBJ): dbg_core/dbg_disasm.c dbg_core/dbg_core.h cpu/cpu.inc cpu/cpu_dummy.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg_core/dbg_disasm.c -o dbg_disasm$(OBJ)

dbg_ui.$(BACKEND)$(OBJ): dbg_ui/dbg_ui.$(BACKEND).c dbg_ui/dbg_ui.h dbg_core/dbg_core.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg_ui/dbg_ui.$(BACKEND).c -o dbg_ui.$(BACKEND)$(OBJ)

# dbg_ui$(OBJ): dbg/dbg_ui.c dbg/dbg.h
# 	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg_ui.c -o dbg_ui$(OBJ)

# dbg_gg$(OBJ): dbg/dbg_gg.c dbg/dbg.h cpu/cpu.h mmu/mmu.h
# 	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg_gg.c -o dbg_gg$(OBJ)

# dbg.$(BACKEND)$(OBJ): dbg/dbg.$(BACKEND).c dbg/dbg.h
# 	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg.$(BACKEND).c -o dbg.$(BACKEND)$(OBJ)

# dbg_disasm$(OBJ): dbg/dbg_disasm.c dbg/dbg.h cpu/cpu.inc cpu/cpu_dummy.h
# 	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg_disasm.c -o dbg_disasm$(OBJ)

mmu$(OBJ): mmu/mmu.c mmu/mmu.h
	$(COMPILER) $(COMPILERFLAGS) -c mmu/mmu.c -o mmu$(OBJ)

gpu$(OBJ): gpu/gpu.c gpu/gpu.h mmu/mmu.h gpu/blit.h
	$(COMPILER) $(COMPILERFLAGS) -c gpu/gpu.c -o gpu$(OBJ)

gfx.$(BACKEND)$(OBJ): gpu/gfx.$(BACKEND).c gpu/gfx.h gpu/blit.h
	$(COMPILER) $(COMPILERFLAGS) -c gpu/gfx.$(BACKEND).c -o gfx.$(BACKEND)$(OBJ)

blit$(OBJ): gpu/blit.c gpu/blit.h
	$(COMPILER) $(COMPILERFLAGS) -c gpu/blit.c -o blit$(OBJ)

main$(OBJ): main.c mmu/mmu.h cpu/cpu.h gpu/gfx.h gpu/gpu.h
	$(COMPILER) $(COMPILERFLAGS) -c main.c -o main$(OBJ)

disasm$(OBJ): disasm.c mmu/mmu.h dbg_core/dbg_core.h
	$(COMPILER) $(COMPILERFLAGS) -c disasm.c -o disasm$(OBJ)

dbg_test$(OBJ): dbg_test.c dbg_core/dbg_core.h dbg_ui/dbg_ui.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg_test.c -o dbg_test$(OBJ)

$(PROGRAM): $(OBJECTS)
	$(LINKER) $(LINKFLAGS) $(OBJECTS) $(GFXLIBRARY) -o $(PROGRAM)

$(LIBRARY): $(LIBRARY_OBJECTS)
	$(LINKER) $(LINKFLAGS) $(SOFLAGS) $(LIBRARY_OBJECTS) $(GFXLIBRARY) -o $(LIBRARY)

$(DISASM_PROGRAM): $(DISASM_OBJECTS)
	$(LINKER) $(LINKFLAGS) $(DISASM_OBJECTS) -o $(DISASM_PROGRAM)

$(DBG_TEST_PROGRAM): $(DBG_TEST_OBJECTS)
	$(LINKER) $(LINKFLAGS) $(DBG_TEST_OBJECTS) $(GFXLIBRARY) -o $(DBG_TEST_PROGRAM)

clean:
	rm $(OBJECTS) || del $(OBJECTS) || echo
	rm $(PROGRAM) || del $(PROGRAM) || echo
	rm $(DISASM_OBJECTS) || del $(DISASM_OBJECTS) || echo
	rm $(DISASM_PROGRAM) || del $(DISASM_PROGRAM) || echo
