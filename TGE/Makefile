install:
	make -C iop/intrelay DEV9_SUPPORT=no RPC_IRQ_SUPPORT=no install
	make -C iop/intrelay DEV9_SUPPORT=no RPC_IRQ_SUPPORT=yes install
	make -C iop/intrelay DEV9_SUPPORT=yes RPC_IRQ_SUPPORT=no install
	make -C iop/intrelay DEV9_SUPPORT=yes RPC_IRQ_SUPPORT=yes install
	make -C iop/dmarelay install
	make ROM_MODULE_VERSION=old -C sbios install
	make ROM_MODULE_VERSION=new -C sbios install

clean:
	make ROM_MODULE_VERSION=old -C sbios clean
	make ROM_MODULE_VERSION=new -C sbios clean
	make -C iop/dmarelay clean
	make -C iop/intrelay DEV9_SUPPORT=no RPC_IRQ_SUPPORT=no clean
	make -C iop/intrelay DEV9_SUPPORT=no RPC_IRQ_SUPPORT=yes clean
	make -C iop/intrelay DEV9_SUPPORT=yes RPC_IRQ_SUPPORT=no clean
	make -C iop/intrelay DEV9_SUPPORT=yes RPC_IRQ_SUPPORT=yes clean
