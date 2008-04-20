CC=ee-gcc
AS=ee-as
LD=ee-elf-ld

include ../config.mk

PROGRAM = kernel
INSTALLFILE = ../loader/kernel.elf

OBJ_DIR = obj

# We specify -G0 to get rid of gp register problems.
CFLAGS = -Wall -W -Werror-implicit-function-declaration -Wimplicit-int -EL -g -G0 -O0 -mips3 -nostdlib -DPS2_EE

SRC = start $(PROGRAM) rtesif strlen strncpy \
	cache timers irq interrupts panic mmu demo loader syscall \
	entry graphic time string exception memory thread intc dmac \
	ledflash

ifeq ($(SHARED_MEM_DEBUG),yes)
CFLAGS += -DSHARED_MEM_DEBUG
SRC += iopmem printf snprintf
endif


OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(SRC)))

install: $(INSTALLFILE)

$(INSTALLFILE): $(PROGRAM).elf
	install -m 644 -p $(PROGRAM).elf $(INSTALLFILE)
	ee-strip $(INSTALLFILE)

$(PROGRAM).elf: $(OBJS)
	$(CC) $(CFLAGS) -Tlinkfile -o $(PROGRAM).elf $(OBJS) -lgcc

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
	bin2s $< $@ demo
