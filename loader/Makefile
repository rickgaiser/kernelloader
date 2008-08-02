PROGRAM = loader
PACKEDFILE = kloader.elf
include ../config.mk

EE_BIN = $(PROGRAM).elf
EE_OBJS = $(PROGRAM).o main.o jump.o entry.o exception.o interrupts.o graphic.o cache.o usb.o ps2dev9.o
EE_OBJS += menu.o menuEntry.o pad.o modules.o rom.o loadermenu.o configuration.o eedebug.o hdd.o
EE_OBJS += kernelgraphic.o SMS_SIF.o SMS_CDVD.o SMS_CDDA.o getrte.o getsbios.o getelf.o
EE_LIBS = -lpatches -L$(PS2DEV)/gsKit/lib -lgskit -ldmakit -L$(PS2SDK)/ports/lib -lstlport
EE_LIBS += -lmc -lfileXio -lkbd
ifneq ($(LIBJPG),)
EE_LIBS += -L$(LIBJPG)/lib -ljpg
endif
ifneq ($(LIBTIFF),)
EE_LIBS += -L$(LIBTIFF) -ltiff
endif
EE_LIBS += -lm -ldebug
EE_FLAGS = -g -I$(PS2DEV)/gsKit/include -I../include
EE_FLAGS += -Wall -W
ifeq ($(SHARED_MEM_DEBUG),yes)
EE_FLAGS += -DSHARED_MEM_DEBUG
EE_OBJS += iopmem.o
endif
ifeq ($(LOAD_PS2LINK),yes)
EE_FLAGS += -DPS2LINK
endif
ifeq ($(LOAD_NAPLINK),yes)
EE_FLAGS += -DNAPLINK
ROM_FILES += $(NAPLINK_MODULES)
endif
ifeq ($(RESET_IOP),yes)
EE_FLAGS += -DRESET_IOP
endif
ifeq ($(SCREENSHOT),yes)
EE_FLAGS += -DSCREENSHOT
endif
ifeq ($(NEW_ROM_MODULES),yes)
EE_FLAGS += -DNEW_ROM_MODULES
EE_LIBS += -lpadx
else
EE_FLAGS += -DOLD_ROM_MODULES
EE_LIBS += -lpad
endif
EE_OBJS += $(addsuffix .o, $(subst .,_,$(ROM_FILES)))
EE_LIBS += -lz

# Autodetect if RTE is available.
EE_FLAGS += $(shell if [ -e "$(PS2LINUXDVD)/pbpx_955.09" ]; then echo -DRTE; fi)

EE_FLAGS += -I$(PS2SDK)/ports/include

EE_CFLAGS += $(EE_FLAGS) -Wimplicit-int -Werror-implicit-function-declaration

EE_CXXFLAGS = $(EE_FLAGS) -D_STLP_NO_NAMESPACES -D_NOTHREADS -D_STLP_NO_EXCEPTIONS -D_STLP_USE_NEWALLOC -D_STLP_HAS_WCHAR_T -D_STLP_NO_IOSTREAMS -Dwint_t=int -I$(PS2SDK)/ports/include/stlport/

MODULES = \
	ps2dev9.irx \
	eedebug.irx \
	fileXio.irx \
	iomanX.irx \
	ioptrap.irx \
	poweroff.irx \
	ps2ip.irx \
	ps2link.irx \
	ps2smap.irx \
	sharedmem.irx \
	usbd.irx \
	usb_mass.irx \
	ps2kbd.irx \
	freesio2.irx \
	freepad.irx \
	freesd.irx \
	audsrv.irx

NAPLINK_MODULES += \
	npm-usbd.irx \
	npm-2301.irx

ROM_FILES += $(MODULES) \
	SMSUTILS.irx \
	SMSCDVD.irx \
	eromdrvloader.irx \
	TGE/dmarelay.irx \
	TGE/intrelay-direct.irx \
	TGE/intrelay-dev9.irx \
	TGE/intrelay-direct-rpc.irx \
	TGE/intrelay-dev9-rpc.irx \
	TGE/sbios_old.elf \
	TGE/sbios_new.elf \
	kernel.elf \
	$(RGB_FILES)

RGB_FILES = \
	folder.rgb \
	penguin.rgb \
	up.rgb \
	back.rgb \
	selected.rgb \
	unselected.rgb \
	disc.rgb \
	cloud.rgb

RGB_HEADERS = \
	$(subst .rgb,.h, $(RGB_FILES))

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

$(NAPLINK_MODULES):
	for file in $(NAPLINK_MODULES); do cp $(NAPLINK_PATH)/$$file .; done

rom.o: rom.c romfilelist.h romdefinitions.h rominitialize.h

test: all
	#ps2client -h $(TARGET_IP) execee host:$(PROGRAM).elf
	ps2client -h $(TARGET_IP) execee host:$(PACKEDFILE)

reset: $(PROGRAM).elf
	ps2client -h $(TARGET_IP) reset

clean:
	rm -f $(EE_BIN) *.o
	rm -f kernel.elf $(MODULES)
	rm -f romfilelist.h romdefinitions.h rominitialize.h
	rm -f $(RGB_HEADERS) $(RGB_FILES)
	rm -f $(PACKEDFILE)
	rm -f $(ROM_FILES)
	rm -f $(addsuffix .s,$(subst .,_,$(ROM_FILES)))
	rm -f $(addsuffix .o,$(subst .,_,$(ROM_FILES)))
	rm -rf .depend
	rm -rf TGE RTE

romfilelist.h: $(RGB_HEADERS) $(RGB_FILES)
	rm -f romfilelist.h
	for file in $(ROM_FILES); do \
		echo "{ \"$$file\", NULL, 0, 0, 0, 0}," >> romfilelist.h; \
	done

romdefinitions.h:
	rm -f romdefinitions.h
	for file in $(subst -,_,$(subst .,_,$(subst /,_,$(ROM_FILES)))); do \
		echo "extern unsigned char $$file[];" >> romdefinitions.h; \
		echo "extern int size_$$file;" >> romdefinitions.h; \
	done

rominitialize.h: $(RGBFILES)
	rm -f rominitialize.h
	for file in $(subst -,_,$(subst .,_,$(subst /,_,$(ROM_FILES)))); do \
		if [ "$${file: -3:3}" = "rgb" ]; then \
			echo "#include \"$${file}\"" | sed -e "s/_rgb\"/\.h\"/g" >> rominitialize.h; \
		fi; \
	done
	for file in $(subst -,_,$(subst .,_,$(subst /,_,$(ROM_FILES)))); do \
		echo "rom_files[i].start = $$file;" >> rominitialize.h; \
		echo "rom_files[i].size = size_$$file;" >> rominitialize.h; \
		if [ "$${file: -3:3}" = "rgb" ]; then \
			ufile="`echo $$file | perl -n -e \"s/_rgb\$$//g; print uc($$@);\"`"; \
			echo "rom_files[i].width = $${ufile}_WIDTH;" >> rominitialize.h; \
			echo "rom_files[i].height = $${ufile}_HEIGHT;" >> rominitialize.h; \
			echo "rom_files[i].depth = $${ufile}_DEPTH;" >> rominitialize.h; \
		fi; \
		echo "i++;" >> rominitialize.h; \
	done

%_irx.s: %.irx
	rm -f $@.tmp
	rm -f $@
	bin2s $^ $@.tmp $(subst -,_,$(subst /,_,$*))_irx
	sed -e "s/\.data/.section .rom/g" <$@.tmp >$@
	rm -f $@.tmp

%_elf.s: %.elf
	bin2s $^ $@ $(subst -,_,$(subst /,_,$*))_elf

%_rgb.s: %.rgb
	rm -f $@.tmp
	rm -f $@
	bin2s $^ $@.tmp $(subst -,_,$(subst /,_,$*))_rgb
	sed -e "s/\.data/.section .rom/g" <$@.tmp >$@
	rm -f $@.tmp

%.rgb %.h: %.ppm ../ppm2rgb/ppm2rgb
	../ppm2rgb/ppm2rgb $*.ppm $*.rgb $*.h $*

%.rgb %.h: %.png ../png2rgb/png2rgb
	../png2rgb/png2rgb $*.png $*.rgb $*.h $*

.depend/%.d: %.c
	mkdir -p .depend
	$(EE_CC) -E -M -MG $(EE_CFLAGS) $(EE_INCS) $< >$@

.depend/%.d: %.cpp
	mkdir -p .depend
	$(EE_CXX) -E -M -MG $(EE_CXXFLAGS) $(EE_INCS) $< >$@

include Makefile.pref
include Makefile.eeglobal
ifneq ($(MAKECMDGOALS),clean)
-include $(addprefix .depend/,$(subst .o,.d, $(EE_OBJS)))
endif
