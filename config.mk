PS2LINUXDVD = /media/dvd
TARGET_IP = 192.168.0.23
EXAMPLE_ELF = ../ps2tut_01/demo1.elf
#EXAMPLE_ELF = ../hello/hello.elf

# Debug output using shared memory (working without RPC).
SHARED_MEM_DEBUG = no
#SHARED_MEM_DEBUG = yes

# No debug, but Linux USB driver is working.
LOAD_PS2LINK = no

# Linux USB driver is not working.
#LOAD_PS2LINK = yes

# Use new rom modules with X or not.
ROM_MODULE_VERSION=old
#ROM_MODULE_VERSION=new

# Debug output using fileio functions (RPC).
FILEIO_DEBUG=no
