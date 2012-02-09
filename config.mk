PS2LINUXDVD = /media/cdrom
TARGET_IP = 192.168.0.23
EXAMPLE_ELF = ../hello/hello.elf

# Set debug output type:
# 1. none - no debug output
# 2. shared - sharedmem.irx via ps2link
# 3. fileio - SIF RPC stdout
# 4. callback - Call function registered by Linux kernel (printk, dmesg).
DEBUG_OUTPUT_TYPE = callback

# Reset IOP at start (only working when enabled)
RESET_IOP = yes

# Activate ps2link debug modules in kernelloader (has only effect when IOP
# reset is done):
LOAD_PS2LINK = no

# Activate if started by naplink
LOAD_NAPLINK = no
NAPLINK_PATH = ../../naplink-ps2-v1.0.1a/cd/

# Use new ROM modules in loader.
NEW_ROM_MODULES = no

# Press button "R1" to get a screenshot on "host:" or "mass0:".
SCREENSHOT = yes

# Activate debug for SBIOS (has only effect with shared memory debug or callback debug).
SBIOS_DEBUG = no

# Activate to be able to move the screen with the analog stick.
PAD_MOVE_SCREEN = yes

### Don't change the following part, change DEBUG_OUTPUT_TYPE instead.

ifeq ($(DEBUG_OUTPUT_TYPE),shared)
# Debug output using shared memory (working without RPC).
SHARED_MEM_DEBUG = yes
else
SHARED_MEM_DEBUG = no
endif

ifeq ($(DEBUG_OUTPUT_TYPE),fileio)
# Needs to be disabled when SHARED_MEM_DEBUG is active.
FILEIO_DEBUG = yes
else
FILEIO_DEBUG = no
endif

ifeq ($(DEBUG_OUTPUT_TYPE),callback)
# Activate printf callback in SBIOS
CALLBACK_DEBUG = yes
else
CALLBACK_DEBUG = no
endif
