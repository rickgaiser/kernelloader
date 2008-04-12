PROGRAM = loader
include ../config.mk

EE_BIN = $(PROGRAM).elf
EE_OBJS = $(PROGRAM).o main.o iopmem.o jump.o entry.o exception.o interrupts.o graphic.o cache.o
EE_LIBS = -lpatches -L$(PS2DEV)/gsKit/lib -lgskit -ldmakit
ifneq ($(LIBJPG),)
EE_LIBS += -L$(LIBJPG)/lib -ljpg
endif
ifneq ($(LIBTIFF),)
EE_LIBS += -L$(LIBTIFF) -ltiff
endif
EE_LIBS += -lm -ldebug
EE_CFLAGS = -g -I$(PS2DEV)/gsKit/include
EE_CFLAGS += -D$(TYPE)

MODULES = ioptrap.irx iomanX.irx ps2dev9.irx ps2ip.irx ps2smap.irx poweroff.irx \
	ps2link.irx

all: $(EE_BIN) modules

modules:
	for file in $(MODULES); do cp $(PS2SDK)/iop/irx/$$file .; done

test: all
	ps2client -h $(TARGET_IP) execee host:$(PROGRAM).elf

reset: $(PROGRAM).elf
	ps2client -h $(TARGET_IP) reset

clean:
	rm -f $(EE_BIN) *.o
	rm -f sbios.elf kernel.elf sharedmem.irx $(MODULES) intrelay.irx dmarelay.irx

include Makefile.pref
include Makefile.eeglobal

