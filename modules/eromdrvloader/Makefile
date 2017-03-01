IOP_OBJS_DIR = ./
IOP_BIN_DIR  = ./
IOP_SRC_DIR  = ./

IOP_BIN  = eromdrvloader.irx
IOP_OBJS = eromdrvloader.o imports.o

all: $(IOP_BIN)
	cp $(IOP_BIN) ../../loader

test: all
	ps2client -h 192.168.0.23 execiop host:$(IOP_BIN) rom1:EROMDRV

reset:
	ps2client -h 192.168.0.23 reset

clean:
	rm -f *.elf *.o *.a *.irx

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release

