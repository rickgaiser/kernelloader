install:
	make -C iop/intrelay install
	make -C iop/dmarelay install
	make ROM_MODULE_VERSION=old -C sbios install
	make ROM_MODULE_VERSION=new -C sbios install

clean:
	make ROM_MODULE_VERSION=old -C sbios clean
	make ROM_MODULE_VERSION=new -C sbios clean
	make -C iop/dmarelay clean
	make -C iop/intrelay clean
