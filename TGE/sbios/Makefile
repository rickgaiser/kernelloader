include ../../config.mk
EE_PREFIX = ee-
EE_CC = $(EE_PREFIX)gcc
EE_OBJCOPY = $(EE_PREFIX)objcopy

EE_INCS = -I. -I../include
EE_CFLAGS = -D_EE -O2 -G0 -Wall -g $(EE_INCS) -Wall -W -Werror-implicit-function-declaration -Wimplicit-int

SBIOS_ADDR = 0x80001000
EE_LDFLAGS = -nostartfiles -Wl,-Ttext -Wl,$(SBIOS_ADDR) -Wl,--defsym -Wl,_start=$(SBIOS_ADDR)

EE_OBJS = prologue.o sbios.o misc.o sifdma.o core.o sifcmd.o sifrpc.o iopheap.o mc.o cache.o \
	strlen.o strncpy.o memcpy.o pad.o memset.o strcpy.o cdvd.o ncmd.o scmd.o
ifeq ($(FILEIO_DEBUG),yes)
EE_CFLAGS += -DFILEIO_DEBUG
EE_OBJS += fileio.o
endif
ifeq ($(SHARED_MEM_DEBUG),yes)
EE_OBJS += printf.o snprintf.o
else
ifeq ($(FILEIO_DEBUG),yes)
EE_OBJS += printf.o snprintf.o
endif
endif
ifeq ($(SHARED_MEM_DEBUG),yes)
EE_CFLAGS += -DSHARED_MEM_DEBUG
EE_OBJS += iopmem.o
endif
ifeq ($(ROM_MODULE_VERSION),old)
EE_CFLAGS += -DOLD_ROM_MODULE_VERSION
else
EE_CFLAGS += -DNEW_ROM_MODULE_VERSION
endif

install: all
	mkdir -p ../../loader/TGE
	cp sbios.elf ../../loader/TGE/

all: sbios.bin

sbios.bin: sbios.elf
	$(EE_OBJCOPY) -I elf32-littlemips -O binary $< $@

sbios.elf: $(EE_OBJS)
	$(EE_CC) $(EE_LDFLAGS) -o $@ $(EE_OBJS)

clean:
	rm -f sbios.bin sbios.elf $(EE_OBJS)

%.o: %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o: %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
