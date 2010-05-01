PS2LINUXDVD = /media/cdrom
TARGET_IP = 192.168.0.23
EXAMPLE_ELF = ../hello/hello.elf

# Debug output using shared memory (working without RPC).
SHARED_MEM_DEBUG = no

# Reset IOP at start (only working when enabled)
RESET_IOP = yes

# Activate ps2link debug modules in kernelloader (has only effect when IOP
# reset is done):
LOAD_PS2LINK = no

# Activate if started by naplink
LOAD_NAPLINK = no
NAPLINK_PATH = ../../naplink-ps2-v1.0.1a/cd/

# Debug output using fileio functions (RPC).
FILEIO_DEBUG = no

# Use new ROM modules in loader.
NEW_ROM_MODULES = no

# Press button "R1" to get a screenshot on "host:" or "mass0:".
SCREENSHOT = yes

# Activate debug for SBIOS (has only effect with shared memory debug).
SBIOS_DEBUG = no

# Activate printf callback in SBIOS
CALLBACK_DEBUG = yes
