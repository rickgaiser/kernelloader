PROGRAM = loader
PACKEDFILE = kloader.elf
include ../config.mk

EE_BIN = $(PROGRAM).elf
EE_OBJS = $(PROGRAM).o main.o jump.o entry.o exception.o interrupts.o graphic.o cache.o usb.o ps2dev9.o
EE_OBJS += menu.o menuEntry.o pad.o modules.o rom.o
EE_LIBS = -lpatches -L$(PS2DEV)/gsKit/lib -lgskit -ldmakit -L$(PS2SDK)/ports/lib -lstlport
EE_LIBS += -lpad -lmc -lfileXio
ifneq ($(LIBJPG),)
EE_LIBS += -L$(LIBJPG)/lib -ljpg
endif
ifneq ($(LIBTIFF),)
EE_LIBS += -L$(LIBTIFF) -ltiff
endif
EE_LIBS += -lm -ldebug
EE_CFLAGS = -g -I$(PS2DEV)/gsKit/include -I../include
EE_CFLAGS += -Wall -W -Werror-implicit-function-declaration -Wimplicit-int
ifeq ($(SHARED_MEM_DEBUG),yes)
EE_CFLAGS += -DSHARED_MEM_DEBUG
EE_OBJS += iopmem.o
endif
ifeq ($(LOAD_PS2LINK),yes)
EE_CFLAGS += -DPS2LINK
endif
ifeq ($(ROM_MODULE_VERSION),old)
EE_CFLAGS += -DOLD_ROM_MODULE_VERSION
else
EE_CFLAGS += -DNEW_ROM_MODULE_VERSION
endif
EE_OBJS += $(addsuffix .o, $(subst .,_,$(ROM_FILES)))

EE_CFLAGS += $(shell if [ -e "$(PS2LINUXDVD)/pbpx_955.09" ]; then echo -DRTE; fi)

EE_CXXFLAGS = $(EE_CFLAGS) -D_STLP_NO_NAMESPACES -D_NOTHREADS -D_STLP_NO_EXCEPTIONS -D_STLP_USE_NEWALLOC -D_STLP_HAS_WCHAR_T -D_STLP_NO_IOSTREAMS -Dwint_t=int -I$(PS2SDK)/ports/include/stlport/

MODULES = ioptrap.irx iomanX.irx ps2dev9.irx ps2ip.irx ps2smap.irx poweroff.irx \
	ps2link.irx usbd.irx usb_mass.irx fileXio.irx

ROM_FILES = \
	eedebug.irx \
	fileXio.irx \
	iomanX.irx \
	ioptrap.irx \
	poweroff.irx \
	ps2dev9.irx \
	ps2ip.irx \
	ps2link.irx \
	ps2smap.irx \
	sharedmem.irx \
	usbd.irx \
	usb_mass.irx \
	TGE/dmarelay.irx \
	TGE/intrelay.irx \
	TGE/sbios.elf \
	kernel.elf

RTE_FILES += \
	RTE/cdvdfsv.irx \
	RTE/cdvdman.irx \
	RTE/dmarelay.irx \
	RTE/iopintr.irx \
	RTE/libsd.irx \
	RTE/mcman.irx \
	RTE/mcserv.irx \
	RTE/padman.irx \
	RTE/sdrdrv.irx \
	RTE/sio2man.irx \
	RTE/sbios.elf

ROM_FILES += $(shell if [ -e "$(PS2LINUXDVD)/pbpx_955.09" ]; then echo $(RTE_FILES); fi)

all: $(PACKEDFILE)

$(PACKEDFILE): $(EE_BIN)
	sjcrunch $^ $@

$(MODULES):
	for file in $(MODULES); do cp $(PS2SDK)/iop/irx/$$file .; done

rom.o: rom.c romfilelist.h romdefinitions.h rominitialize.h

test: all
	#ps2client -h $(TARGET_IP) execee host:$(PROGRAM).elf
	ps2client -h $(TARGET_IP) execee host:$(PACKEDFILE)

reset: $(PROGRAM).elf
	ps2client -h $(TARGET_IP) reset

clean:
	rm -f $(EE_BIN) *.o
	rm -f sbios.elf kernel.elf sharedmem.irx $(MODULES) intrelay.irx dmarelay.irx iopintr.irx cdvdman.irx cdvdfsv.irx
	rm -f romfilelist.h romdefinitions.h rominitialize.h
	rm -f $(PACKEDFILE)

romfilelist.h:
	rm -f romfilelist.h
	for file in $(ROM_FILES); do \
		echo "{ \"$$file\", NULL, 0}," >> romfilelist.h; \
	done

romdefinitions.h:
	rm -f romdefinitions.h
	for file in $(subst .,_,$(subst /,_,$(ROM_FILES))); do \
		echo "extern unsigned char $$file[];" >> romdefinitions.h; \
		echo "extern int size_$$file;" >> romdefinitions.h; \
	done

rominitialize.h:
	rm -f rominitialize.h
	for file in $(subst .,_,$(subst /,_,$(ROM_FILES))); do \
		echo "rom_files[i].start = $$file;" >> rominitialize.h; \
		echo "rom_files[i].size = size_$$file;" >> rominitialize.h; \
		echo "i++;" >> rominitialize.h; \
	done

%_irx.s: %.irx
	bin2s $^ $@ $(subst /,_,$*)_irx

%_elf.s: %.elf
	bin2s $^ $@ $(subst /,_,$*)_elf

include Makefile.pref
include Makefile.eeglobal

