IOP_BIN = eromdrvloader.irx
IOP_OBJS = eromdrvloader.o imports.o
IOP_LDFLAGS = -Wl,-s
IOP_CFLAGS = -D_PS2_

all: $(IOP_BIN)
	cp $(IOP_BIN) ../../loader

test: all
	ps2client -h 192.168.0.23 execiop host:$(IOP_BIN)

reset:
	ps2client -h 192.168.0.23 reset

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
