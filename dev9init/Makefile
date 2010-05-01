IOP_BIN = dev9init.irx
IOP_OBJS = dev9init.o hdd.o imports.o
IOP_LDFLAGS = -Wl,-s
IOP_CFLAGS = -D_PS2_ -I../include -W -Wall -Wimplicit-int -Werror-implicit-function-declaration

install: all
	install -m 644 $(IOP_BIN) ../loader/

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
