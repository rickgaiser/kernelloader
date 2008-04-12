EE_PREFIX = ee-
EE_CC = $(EE_PREFIX)gcc
EE_OBJCOPY = $(EE_PREFIX)objcopy
EE_OBJS = sbios.o
CFLAGS = -Wall -g
include ../config.mk

SBIOS_ADDR = 0x80001000
VIRT_OFFSET = $(shell ./getVirtOffset.pl $(PS2LINUXDVD)/pbpx_955.09)
EE_LDFLAGS = -nostartfiles -Wl,-Ttext -Wl,$(SBIOS_ADDR) -Wl,--defsym -Wl,_start=$(SBIOS_ADDR)

INSTALL_FILES = sbios.elf cdvdman.irx cdvdfsv.irx iopintr.irx dmarelay.irx \
	libsd.irx mcman.irx mcserv.irx padman.irx sdrdrv.irx sio2man.irx

install: all
	mkdir -p ../loader/RTE
	for file in $(INSTALL_FILES); do \
		cp $$file ../loader/RTE/; \
	done

all: $(INSTALL_FILES)

sbios.bin: getsbios getsbioscalltableoffset
	./getsbios $(PS2LINUXDVD) $(VIRT_OFFSET)
	./getsbioscalltableoffset $@

cdvdman.irx: getelf
	./getelf $(PS2LINUXDVD) $@ 3

cdvdfsv.irx: getelf
	./getelf $(PS2LINUXDVD) $@ 4

%.irx: $(PS2LINUXDVD)/modules/mod203/%.irx
	cp $^ $@
	chmod +rw $@

sbios.S: sbios.bin
	bin2s $^ $@ sbios
	patch <sbios.patch

sbios.elf: $(EE_OBJS)
	$(EE_CC) $(EE_LDFLAGS) -o $@ $(EE_OBJS)

clean:
	rm -f sbios.bin sbios.S sbios.o getsbios getelf getsbioscalltableoffset $(INSTALL_FILES)

%.o: %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
