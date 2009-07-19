#  _____     ___ ____
#   ____|   |    ____|      PSX2 OpenSource Project
#  |     ___|   |____       (C)2002, David Ryan ( Oobles@hotmail.com )
#                           (C)2003, adresd     ( adresd_ps2dev@yahoo.com )
#  ------------------------------------------------------------------------


IOP_BIN = smaprpc.irx
IOP_OBJS = main.o smap.o smap_rpc.o imports.o

ifeq ($(DEBUG),1)
DEBUG_FLAGS = -DDEBUG
endif

IOP_INCS += -I$(PS2SDK)/iop/include
IOP_CFLAGS += -Wall -fno-builtin $(DEBUG_FLAGS)
IOP_LDFLAGS += -s

install: all
	cp $(IOP_BIN) ../loader

all: $(IOP_BIN)

clean:
	-rm -f *.o $(IOP_BIN)

include $(PS2SDK)/Defs.make
include Rules.make

