include config.mk

all:
	make -C ppm2rgb
	make -C png2rgb
	make -C hello
	make -C kernel
	make -C sharedmem
	make -C eedebug
	make -C smaprpc
	make -C dev9init
	make -C TGE
	if [ -e $(PS2LINUXDVD)/pbpx_955.09 ]; then \
		make -C RTE; \
	fi
	make -C modules
	make -C loader

test:
	make -C loader test

reset:
	make -C loader reset

clean:
	make -C kernel clean
	make -C hello clean
	make -C sharedmem clean
	make -C eedebug clean
	make -C dev9init clean
	make -C smaprpc clean
	make -C TGE clean
	make -C RTE clean
	make -C modules clean
	make -C loader clean
	make -C ppm2rgb clean
	make -C png2rgb clean
