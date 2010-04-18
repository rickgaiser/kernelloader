include ../../config.mk
EE_PREFIX = ee-
EE_CC = $(EE_PREFIX)gcc
EE_OBJCOPY = $(EE_PREFIX)objcopy

# Default value, overwritten by calling makefile.
ROM_MODULE_VERSION=old

EE_INCS = -I. -I../include -I../../include
EE_CFLAGS = -D_EE
EE_CFLAGS += -G0 -Wall -g
ifeq ($(SBIOS_DEBUG),yes)
# Debug code need more memory, so optimize for size.
EE_CFLAGS += -Os
# Optimize for speed.
else
EE_CFLAGS += -O2
endif
EE_CFLAGS += $(EE_INCS)
EE_CFLAGS += -Wall -W -Werror-implicit-function-declaration -Wimplicit-int -Werror
EE_CFLAGS += -fno-builtin-memcpy -fno-builtin-memset
# BSD doesn't enable floating point registers. Only printf() saves them as precaution.
EE_CFLAGS += -msoft-float

SBIOS_ADDR = 0x80001000
EE_LDFLAGS = -nostartfiles -Wl,-Ttext -Wl,$(SBIOS_ADDR) -Wl,--defsym -Wl,_start=$(SBIOS_ADDR)

EE_FILES = prologue sbios misc sifdma core sifcmd sifrpc iopheap cache \
	strlen strncpy memcpy pad memset strcpy cdvd ncmd scmd smod iopmem \
	memcmp mc mutex sound dve_reg
EE_OBJS = $(addsuffix _$(ROM_MODULE_VERSION).o, $(EE_FILES))
ifeq ($(FILEIO_DEBUG),yes)
EE_CFLAGS += -DFILEIO_DEBUG
EE_FILES += fileio
endif
ifeq ($(SHARED_MEM_DEBUG),yes)
EE_FILES += printf snprintf
else
ifeq ($(FILEIO_DEBUG),yes)
EE_FILES += printf snprintf
else
ifeq ($(CALLBACK_DEBUG),yes)
EE_FILES += printf snprintf
EE_CFLAGS += -DCALLBACK_DEBUG -fno-builtin
endif
endif
endif
ifeq ($(SHARED_MEM_DEBUG),yes)
EE_CFLAGS += -DSHARED_MEM_DEBUG
EE_FILES += iopmemdebug
endif
ifeq ($(ROM_MODULE_VERSION),old)
EE_CFLAGS += -DOLD_ROM_MODULE_VERSION
else
EE_CFLAGS += -DNEW_ROM_MODULE_VERSION
endif
ifeq ($(SBIOS_DEBUG),yes)
EE_CFLAGS += -DSBIOS_DEBUG
endif

install: all
	mkdir -p ../../loader/TGE
	install -m 644 -p sbios_$(ROM_MODULE_VERSION).elf ../../loader/TGE/
	ee-strip ../../loader/TGE/sbios_$(ROM_MODULE_VERSION).elf

all: sbios_$(ROM_MODULE_VERSION).bin

sbios_$(ROM_MODULE_VERSION).bin: sbios_$(ROM_MODULE_VERSION).elf
	$(EE_OBJCOPY) -I elf32-littlemips -O binary $< $@

sbios_$(ROM_MODULE_VERSION).elf: $(EE_OBJS)
	$(EE_CC) $(EE_LDFLAGS) -o $@ $(EE_OBJS)

clean:
	rm -f sbios_$(ROM_MODULE_VERSION).bin sbios_$(ROM_MODULE_VERSION).elf $(EE_OBJS)

%_$(ROM_MODULE_VERSION).o: %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%_$(ROM_MODULE_VERSION).o: %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@
