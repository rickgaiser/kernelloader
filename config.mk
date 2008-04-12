# SBIOS type used:
TYPE = TGE
# XXX: not working with linux.

#TYPE = RTE

PS2LINUXDVD = /media/dvd
TARGET_IP = 192.168.0.23
#EXAMPLE_ELF = ../ps2tut_01/demo1.elf
EXAMPLE_ELF = ../hello/hello.elf
SHARED_MEM_DEBUG = no
#SHARED_MEM_DEBUG = yes

# No debug, but Linux USB driver is working.
LOAD_PS2LINK = no

# Linux USB driver is not working.
#LOAD_PS2LINK = yes

