include config.mk

all:
	make -C ps2tut_01
	make -C hello
	make -C kernel
	make -C sharedmem
	make -C eedebug
	make -C TGE
	if [ -e $(PS2LINUXDVD)/pbpx_955.09 ]; then \
		make -C RTE; \
	fi
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
	make -C eedebug clean
	make -C TGE clean
	make -C RTE clean
	make -C loader clean
