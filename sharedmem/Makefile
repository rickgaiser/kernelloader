IOP_OBJS_DIR = ./
IOP_BIN_DIR  = ./
IOP_SRC_DIR  = ./

IOP_BIN = sharedmem.irx
IOP_OBJS = sharedmem.o imports.o
IOP_LDFLAGS = -Wl,-s
IOP_CFLAGS = -D_PS2_ -I../include

install: all
	cp $(IOP_BIN) ../loader

all: $(IOP_BIN)

$(IOP_BIN): $(IOP_OBJS)

clean:
	rm -f *.elf *.o *.a *.irx imports.c

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
