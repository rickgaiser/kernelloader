IOP_BIN = eedebug.irx
IOP_OBJS = eedebug.o imports.o
IOP_LDFLAGS = -Wl,-s
IOP_CFLAGS = -D_PS2_ -I../include

install: all
	cp $(IOP_BIN) ../loader

all: $(IOP_BIN)

clean:
	rm -f *.elf *.o *.a *.irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.iopglobal

# Remove stupid "-c" from IOP_CFLAGS
IOP_CFLAGS := $(filter-out -c,$(IOP_CFLAGS))

# A rule to build imports.lst.
%.c : %.lst
	echo "#include \"irx_imports.h\"" > imports.c
	cat $< >> imports.c
