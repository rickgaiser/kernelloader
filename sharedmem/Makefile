IOP_BIN = sharedmem.irx
IOP_OBJS = sharedmem.o imports.o
IOP_LDFLAGS = -Wl,-s
IOP_CFLAGS = -D_PS2_

install: all
	cp $(IOP_BIN) ../loader

all: $(IOP_BIN)

$(IOP_BIN): $(IOP_OBJS)

clean:
	rm -f *.elf *.o *.a *.irx imports.c

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal

# A rule to build imports.lst.
%.c : %.lst
	echo "#include \"irx_imports.h\"" > imports.c
	cat $< >> imports.c
