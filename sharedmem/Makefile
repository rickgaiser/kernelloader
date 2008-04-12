IOP_BIN = sharedmem.irx
IOP_OBJS = sharedmem.o imports.o
IOP_LDFLAGS = -Wl,-s
IOP_CFLAGS = -D_PS2_

install: all
	cp $(IOP_BIN) ../loader

all: $(IOP_BIN)

clean:
	rm -f *.elf *.o *.a *.irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal

# A rule to build imports.lst.
%.o : %.lst
	echo "#include \"irx_imports.h\"" > build-imports.c
	cat $< >> build-imports.c
	$(IOP_CC) $(IOP_CFLAGS) build-imports.c -o $@
	-rm -f build-imports.c
