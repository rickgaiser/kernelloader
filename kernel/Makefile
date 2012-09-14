include ../config.mk

ifeq ($(NEW_KERNEL_TOOLCHAIN),yes)
CROSS_COMPILE = mips64el-linux-gnu-
BIN2S=/usr/local/ps2dev/ps2sdk/bin/bin2s
LINKSCRIPT = elf32ltsmipn32.x
else
CROSS_COMPILE = ee-
BIN2S=bin2s
LINKSCRIPT = linkfile
endif
CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip

PROGRAM = kernel
INSTALLFILE = ../loader/kernel.elf

OBJ_DIR = obj

CFLAGS = -Wall -W -Werror-implicit-function-declaration -Wimplicit-int -EL -g \
	-nostdlib -I../include

ifeq ($(NEW_KERNEL_TOOLCHAIN),yes)
CFLAGS += -fno-builtin -mabi=n32 -march=r5900 -mno-llsc
else
CFLAGS += -mips3 -G0 -O0 -DPS2_EE
endif

SRC = start $(PROGRAM) rtesif strlen strncpy \
	cache timers irq interrupts panic mmu demo loader syscall \
	entry graphic time string exception memory thread intc dmac \
	ledflash memcmp

ifeq ($(SHARED_MEM_DEBUG),yes)
CFLAGS += -DSHARED_MEM_DEBUG
SRC += iopmem printf snprintf smod
endif


OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(SRC)))

install: $(INSTALLFILE)

$(INSTALLFILE): $(PROGRAM).elf
	install -m 644 -p $(PROGRAM).elf $(INSTALLFILE)
	$(STRIP) $(INSTALLFILE)

$(PROGRAM).elf: $(OBJS)
	$(CC) $(CFLAGS) -T$(LINKSCRIPT) -o $(PROGRAM).elf $(OBJS) -lgcc

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(OBJ_DIR)
	$(CC) -DASSEMBLER -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(PROGRAM).o $(PROGRAM).elf demo.S
	-rmdir $(OBJ_DIR)

demo.S: $(EXAMPLE_ELF)
	$(BIN2S) $< $@ demo
