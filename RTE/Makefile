EE_PREFIX = ee-
EE_CC = $(EE_PREFIX)gcc
EE_OBJCOPY = $(EE_PREFIX)objcopy
EE_OBJS = sbios.o
CFLAGS = -Wall -g
include ../config.mk

SBIOS_ADDR = 0x80001000
VIRT_OFFSET = $(shell ./getVirtOffset.pl $(PS2LINUXDVD)/pbpx_955.09)
EE_LDFLAGS = -nostartfiles -Wl,-Ttext -Wl,$(SBIOS_ADDR) -Wl,--defsym -Wl,_start=$(SBIOS_ADDR)

install: all
	cp sbios.elf ../loader/

all: sbios.elf

sbios.bin: getsbios
	./getsbios $(PS2LINUXDVD) $(VIRT_OFFSET)

sbios.S: sbios.bin
	bin2s $^ $@ sbios
	patch <sbios.patch

sbios.elf: $(EE_OBJS)
	$(EE_CC) $(EE_LDFLAGS) -o $@ $(EE_OBJS)

clean:
	rm -f sbios.S sbios.o sbios.elf getsbios

%.o: %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
