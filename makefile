
YASMFLAGS=-m $(ARCH) -f $(PLATFORM)

PROGRAM=gg$(EXE)
LIBRARY=gg$(SO)
DISASM_PROGRAM=gg_disasm$(EXE)
DBG_TEST_PROGRAM=gg_dbg_test$(EXE)
LIBRARY_OBJECTS=mmu$(OBJ) dbg_disasm$(OBJ) dbg_gg$(OBJ) dbg_ui$(OBJ) dbg.$(BACKEND)$(OBJ) gpu$(OBJ) blit$(OBJ) gfx.$(BACKEND)$(OBJ)
OBJECTS=main$(OBJ) cpu$(OBJ) $(LIBRARY_OBJECTS)
DISASM_OBJECTS=mmu$(OBJ) dbg_disasm$(OBJ) disasm$(OBJ)
DBG_TEST_OBJECTS=dbg_ui$(OBJ) dbg.$(BACKEND)$(OBJ) dbg_test$(OBJ)

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

dbg_ui$(OBJ): dbg/dbg_ui.c dbg/dbg.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg_ui.c -o dbg_ui$(OBJ)

dbg_gg$(OBJ): dbg/dbg_gg.c dbg/dbg.h cpu/cpu.h mmu/mmu.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg_gg.c -o dbg_gg$(OBJ)

dbg.$(BACKEND)$(OBJ): dbg/dbg.$(BACKEND).c dbg/dbg.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg.$(BACKEND).c -o dbg.$(BACKEND)$(OBJ)

dbg_disasm$(OBJ): dbg/dbg_disasm.c dbg/dbg.h cpu/cpu.inc cpu/cpu_dummy.h
	$(COMPILER) $(COMPILERFLAGS) -c dbg/dbg_disasm.c -o dbg_disasm$(OBJ)

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

disasm$(OBJ): disasm.c mmu/mmu.h dbg/dbg.h
	$(COMPILER) $(COMPILERFLAGS) -c disasm.c -o disasm$(OBJ)

dbg_test$(OBJ): dbg_test.c dbg/dbg.h
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