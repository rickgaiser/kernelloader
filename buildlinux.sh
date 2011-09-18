#!/bin/bash
# This script builds Linux 2.4 for PS2

BASEDIR="`pwd`"

error_exit()
{
	echo >&2 "Build error in $BASH_SOURCE at line $BASH_LINENO:"
	# Print command line with error:
	cat -n "$BASEDIR/$BASH_SOURCE" | head -n $[ $BASH_LINENO + 1 ] | tail -n 4
	exit -1
}

cd "`dirname \"$0\"`" || error_exit
SCRIPTDIR="`pwd`"
KLOADERDIR="$SCRIPTDIR"
BUILDDIR="$BASEDIR/build"
SRCDIR="$BASEDIR/source"
INSTALLER=n
PATCHDIR="$KLOADERDIR/patches"
PS2LINUXPREFIX="/usr/local"
PS2LINUXDIR="$PS2LINUXPREFIX/ps2"
NUM_CPUS=`grep /proc/cpuinfo -e "processor" | wc -l`

install_program()
{
	PROG="$1"
	if [ "$INSTALLER" = "y" ]; then
		sudo apt-get install "$PROG"
	else
		echo "Please install program $PROG and press return."
		read
	fi
}

check_program()
{
	PROG="$1"
	which "$PROG" >/dev/null
	if [ $? -ne 0 ]; then
		install_program "$PROG"
		which "$PROG" >/dev/null
		if [ $? -ne 0 ]; then
			error_exit
		fi
	fi
}

which apt-get >/dev/null
if [ $? -eq 0 ]; then
	INSTALLER="y"
fi
check_program wget
check_program uname

SYSTEMTYPE="`uname -m`"
if [ "$SYSTEMTYPE" = "x86_64" ]; then
check_program dchroot
fi

download_file()
{
	FILE="$1"
	URL="$2"

	if [ ! -e "$SRCDIR/$FILE" ]; then
		wget -O "$SRCDIR/$FILE" "$URL" || error_exit
	fi
}

ask_remove()
{
	DIR="$1"
	echo "The path at $DIR must be deleted. Enter "yes" and press return to continue."
	read answer
	if [ "$answer" = "yes" ]; then
		sudo rm -r "$DIR" || error_exit
	fi
}

cd "$BASEDIR" || error_exit
if [ ! -d "$SRCDIR" ]; then
	mkdir "$SRCDIR" || error_exit
fi
if [ -d "$BUILDDIR" ]; then
	rm -r "$BUILDDIR" || error_exit
fi
mkdir "$BUILDDIR" || error_exit

cd "$SRCDIR" || error_exit

download_file binutils-2.9EE-cross.tar.gz http://sourceforge.net/projects/kernelloader/files/Sony%20Linux%20Toolkit/Package%20Update%20Files/ps2stuff/binutils-2.9EE-cross.tar.gz/download
download_file gcc-2.95.2-cross.tar.gz http://sourceforge.net/projects/kernelloader/files/Sony%20Linux%20Toolkit/Package%20Update%20Files/ps2stuff/gcc-2.95.2-cross.tar.gz/download
download_file linux-2.4.17_ps2.tar.bz2 http://sourceforge.net/projects/kernelloader/files/Linux%202.4/Linux%202.4.17%20Kernel%20Source/linux-2.4.17_ps2.tar.bz2/download

if [ -d "$PS2LINUXDIR" ]; then
	ask_remove "$PS2LINUXDIR"
fi
if [ ! -d "$PS2LINUXDIR" ]; then
	sudo mkdir -p "$PS2LINUXPREFIX" || error_exit
	sudo tar -C "$PS2LINUXPREFIX" -xzf "$SRCDIR/binutils-2.9EE-cross.tar.gz" || error_exit
	sudo tar -C "$PS2LINUXPREFIX" -xzf "$SRCDIR/gcc-2.95.2-cross.tar.gz" || error_exit
fi

# Configure Linux 2.4 source code
cd "$BUILDDIR" || error_exit
tar -xjf "$SRCDIR/linux-2.4.17_ps2.tar.bz2" || error_exit
cd linux-2.4.17_ps2 || error_exit
patch -p1 <"$PATCHDIR/linux-2.4.17_ps2-all_fat_and_slim.patch" || error_exit
patch -p1 <"$PATCHDIR/linux-2.4.17_ps2-no-bwlinux-check.patch" || error_exit
patch -p1 <"$PATCHDIR/linux-2.4.17_ps2-nfsroot-via-tcp.patch" || error_exit
cp "$KLOADERDIR/driver_slim_smaprpc/smaprpc.c" drivers/ps2/ || error_exit
cp "$KLOADERDIR/driver_slim_smaprpc/smaprpc.h" drivers/ps2/ || error_exit
cp -r "$KLOADERDIR/driver_ps2fs/ps2fs" fs/ || error_exit
cp -r "$KLOADERDIR/driver_unionfs/unionfs" fs/ || error_exit

echo "$PS2LINUXDIR/bin/ee-" >.hhl_cross_compile-mips-r5900 || error_exit
./setup-ps2 || error_exit

cp "$KLOADERDIR/kernelconfig" .config || error_exit
if [ "$SYSTEMTYPE" = "x86_64" ]; then
	# Need to build with 32 bit Linux system (dchroot):
	dchroot -d "make oldconfig" || error_exit
	dchroot -d "make dep" || error_exit
	dchroot -d "make -j $NUM_CPUS vmlinux" || error_Exit
else
	make oldconfig || error_exit
	make dep || error_exit
	make -j $NUM_CPUS vmlinux || error_Exit
fi
