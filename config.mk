PS2LINUXDVD = /media/dvd
TARGET_IP = 192.168.0.10
EXAMPLE_ELF = ../ps2tut_01/demo1.elf
#EXAMPLE_ELF = ../hello/hello.elf

# Debug output using shared memory (working without RPC).
SHARED_MEM_DEBUG = no

# Reset IOP at start
RESET_IOP = yes

# Activate ps2link debug modules in kernelloader (has only effect when no IOP
# reset is done):
LOAD_PS2LINK = no

# Debug output using fileio functions (RPC).
FILEIO_DEBUG = no

SCREENSHOT = no
