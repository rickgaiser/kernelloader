#----------------------------------------------------------------------------
# File:		Makefile
# Author:	Tony Saveski, t_saveski@yahoo.com
#----------------------------------------------------------------------------
CC=ee-gcc
AS=ee-as
LD=ee-elf-ld

OBJ_DIR = obj
CFLAGS = -Wall -W -EL -G0 -O0 -mips3 -nostdlib -DPS2_EE

C_SRC = g2.c demo1.c
S_SRC = crt0.s ps2_asm.s dma_asm.s gs_asm.s

C_OBJ = $(addprefix $(OBJ_DIR)/, $(C_SRC:.c=.o))
S_OBJ = $(addprefix $(OBJ_DIR)/, $(S_SRC:.s=.o))

demo1.elf: $(C_OBJ) $(S_OBJ)
	@echo "-------------------------------------------------"
	$(CC) $(CFLAGS) -Tlinkfile -o demo1.elf $(C_OBJ) $(S_OBJ) -Wl,-Map,demo1.map

$(OBJ_DIR)/%.o: %.c
	@echo "-------------------------------------------------"
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/%.o: %.s
	@echo "-------------------------------------------------"
	mkdir -p $(OBJ_DIR)
	$(CC) -xassembler-with-cpp -c $(CFLAGS) $< -o $@

clean:
	rm -f $(C_OBJ) $(S_OBJ) *.map *.elf
