
all: dbg.dll

WCCFLAGS=-ox -zw -bd -br -6r -we -wx

dbg_ui.obj: dbg_ui.c dbg.h
	wcc386 dbg_ui.c $(WCCFLAGS)

dbg.win32.obj: dbg.win32.c dbg.h
	wcc386 dbg.win32.c $(WCCFLAGS)

OBJECTS=dbg_ui.obj dbg.win32.obj

dbg.dll: $(OBJECTS)
	wlink op map SYS nt_dll FILE { $(OBJECTS) } NAME dbg.dll
