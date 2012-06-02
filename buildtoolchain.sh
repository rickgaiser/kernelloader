#!/bin/bash
# Build homebrew ps2dev toolchain for use with kernelloader
BASEDIR="`pwd`"

error_exit()
{
	echo >&2 "Build error in $BASH_SOURCE at line $BASH_LINENO:"
	# Print command line with error:
	cat -n "$BASEDIR/$BASH_SOURCE" | head -n $[ $BASH_LINENO + 1 ] | tail -n 4
	exit -1
}

cd "`dirname \"$0\"`" || error_exit

set -x

SCRIPTDIR="`pwd`"
KLOADERDIR="$SCRIPTDIR"
PATCHDIR="$KLOADERDIR/patches"
BUILDDIR="$BASEDIR/build"
INSTALLER=n
NUM_CPUS=`grep /proc/cpuinfo -e "processor" | wc -l`

install_program()
{
	local PROG

	PROG="$1"
	if [ "$PROG" = "svn" ]; then
		PROG="subversion"
	fi
	if [ "$INSTALLER" = "y" ]; then
		sudo apt-get install "$PROG"
	else
		echo "Please install program $PROG and press return."
		read
	fi
}

check_program()
{
	local PROG

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

	sudo apt-get install gcc make liblzo2-dev automake autoconf perl e2fsprogs gzip tar lzma libpng-dev || error_exit
	sudo apt-get build-dep binutils gcc || error_exit
fi
check_program git
check_program wget
check_program svn

cd "$BASEDIR" || error_exit
if [ -d "$BUILDDIR" ]; then
	rm -Ir "$BUILDDIR" || error_exit
fi
mkdir "$BUILDDIR" || error_exit

# PS2 Toolchain
export PS2DEV="/usr/local/ps2dev"
export PS2SDK="$PS2DEV/ps2sdk"
export PATH="${PS2DEV}/bin:${PS2DEV}/ee/bin:${PS2DEV}/iop/bin:${PS2DEV}/dvp/bin:${PS2DEV}/ps2sdk/bin:$PATH"

cat >"$BASEDIR/PS2SDK.sh" <<EOF
export PS2DEV="$PS2DEV"
export PS2SDK="\$PS2DEV/ps2sdk"
export PATH="\${PS2DEV}/bin:\${PS2DEV}/ee/bin:\${PS2DEV}/iop/bin:\${PS2DEV}/dvp/bin:\${PS2DEV}/ps2sdk/bin:\$PATH"
export PS2LIB="\$PS2SDK"

export PS2_SRC_PREFIX="$BUILDDIR"
export PS2SDKSRC="\${PS2_SRC_PREFIX}/ps2sdk"
export PS2ETH="\${PS2_SRC_PREFIX}/ps2eth"
EOF

cd "$BUILDDIR" || error_exit
git clone https://github.com/ps2dev/ps2toolchain.git || error_exit
cd ps2toolchain || error_exit
git checkout ae6f9c003c98470040e78cec739ea86a49de2a39 || error_exit
if [ -d "$PS2DEV" ]; then
	sudo rm -Ir "$PS2DEV" || error_exit
fi
sudo mkdir -p "$PS2DEV" || error_exit
sudo chown "$USER" "$PS2DEV" || error_exit
bash ./toolchain.sh || error_exit

# PS2SDK
cd "$BUILDDIR" || error_exit
git clone https://github.com/ps2dev/ps2sdk.git || error_exit
cd ps2sdk || error_exit
git checkout 38267d0159d686b6e424221ccc2f3cea80b21f8c || error_exit
patch -p0 <"$PATCHDIR/ps2sdk-readnvm.patch" || error_exit
make || error_exit
make install || error_exit

export PS2LIB="$PS2SDK"

export PS2_SRC_PREFIX="$BUILDDIR"
export PS2SDKSRC="${PS2_SRC_PREFIX}/ps2sdk"
export PS2ETH="${PS2_SRC_PREFIX}/ps2eth"

mkdir -p "$PS2_SRC_PREFIX" || error_exit


# USB
cd "$BUILDDIR" || error_exit
svn co -r 1601 http://psp.jim.sh/svn/ps2/trunk/usb_mass || error_exit
cd usb_mass || error_exit
cd iop || error_exit
make || error_exit
cp usb_mass.irx "$PS2SDK/iop/irx" || error_exit

# STL
cd "$BUILDDIR" || error_exit
git clone https://github.com/ps2dev/ps2sdk-ports.git || error_exit
cd ps2sdk-ports || error_exit
git checkout 256d65f916d974d79a382e118be365b62273c9ac || error_exit

cd stlport || error_exit
make || error_exit
make install || error_exit
cd .. || error_exit

# ZLIB
cd zlib || error_exit
make || error_exit
make install || error_exit

# PS2LINK
cd "$BUILDDIR" || error_exit
git clone https://github.com/ps2dev/ps2eth.git || error_exit
cd ps2eth || error_exit
git checkout 2eb548cea263995f35b5bb0a2451a96871a59994 || error_exit
make || error_exit
cp smap/ps2smap.irx "$PS2SDK/iop/irx/" || error_exit
cd .. || error_exit
git clone https://github.com/ps2dev/ps2link.git || error_exit
cd ps2link || error_exit
git checkout 3c6c81637293b93edd1af23f6c0f501af9948352 || error_exit
make iop || error_exit

# It may stop with an error that ps2-packer is missing. It is not
# needed to build it completely. Just continue:

cp iop/ps2link.irx "$PS2SDK/iop/irx/" || error_exit
cd .. || error_exit

# GSKIT
cd "$BUILDDIR" || error_exit
git clone https://github.com/ps2dev/gsKit.git || error_exit
export GSKITSRC="${PS2_SRC_PREFIX}/gsKit"
cd gsKit || error_exit
git checkout 946922b3c15498b3c1a36203195964ddf2fe46e7 || error_exit
make || error_exit
make install || error_exit

# SJCRUNCH
cd "$BUILDDIR" || error_exit
wget http://sksapps.com/mis/sjcrunch-2.1.zip || error_exit
mkdir sjcrunch || error_exit
cd sjcrunch || error_exit
unzip ../sjcrunch-2.1.zip || error_exit
cd pc || error_exit

patch -p0 <"$PATCHDIR/sjcrunch_lzo2.patch" || error_exit

make || error_exit
cd .. || error_exit
cp pc/sjcrunch_packer script || error_exit
cd script || error_exit
sed <sjcrunch -e s-SJCRUNCH_PATH=.*-SJCRUNCH_PATH=$(pwd)- -e s-SJCRUNCH_PACKER=.*-SJCRUNCH_PACKER=\$SJCRUNCH_PATH/sjcrunch_packer- >"$PS2SDK/bin/sjcrunch" || error_exit
chmod +x "$PS2SDK/bin/sjcrunch" || error_exit

