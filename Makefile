include config.mk

all:
	make -C ps2tut_01
	make -C hello
	make -C kernel
ifeq ($(SHARED_MEM_DEBUG),yes)
	make -C sharedmem
endif
	make -C $(TYPE)
	make -C loader

test:
	make -C loader test

reset:
	make -C loader reset

clean:
	make -C kernel clean
	make -C ps2tut_01 clean
	make -C hello clean
	make -C sharedmem clean
	make -C TGE clean
	make -C RTE clean
	make -C loader clean
