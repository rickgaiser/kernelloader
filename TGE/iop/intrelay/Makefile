# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004.
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

IOP_BIN_DIR = bin/
IOP_SRC_DIR = src/
IOP_INC_DIR = include/

DEV9_SUPPORT = no
RPC_IRQ_SUPPORT = no
ifeq ($(RPC_IRQ_SUPPORT),yes)
IRQ_TYPE=-rpc
IOP_CFLAGS += -DRPC_IRQ_SUPPORT
else
IRQ_TYPE=
endif
ifeq ($(DEV9_SUPPORT),yes)
DEV9_TYPE=-dev9

IOP_CFLAGS += -DDEV9_SUPPORT
IOP_INCS += -I$(PS2SDKSRC)/iop/dev9/dev9/include
else
DEV9_TYPE=-direct
endif
IOP_OBJS_DIR = obj$(DEV9_TYPE)$(IRQ_TYPE)/
IOP_BIN  = bin/intrelay$(DEV9_TYPE)$(IRQ_TYPE).irx

IOP_MODS = intrelay imports
IOP_OBJS = $(addsuffix .o, $(addprefix $(IOP_OBJS_DIR), $(IOP_MODS)))
IOP_INCS += -I../../include
IOP_CFLAGS  += -Wall -fno-builtin-printf -fno-builtin-memcpy -Wimplicit-int -Werror-implicit-function-declaration -Werror
IOP_LDFLAGS += -s

install: all
	mkdir -p ../../../loader/TGE
	install -m 644 -p $(IOP_BIN) ../../../loader/TGE

all: $(IOP_OBJS_DIR) $(IOP_BIN_DIR) $(IOP_BIN)

clean:
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
