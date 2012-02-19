Kernelloader for PS2 Linux
##########################

This program can load a Linux kernel and start it on the Sony Playstation 2.
The program is working with the old fat PS2 and the new slim PSTwo. The
support for the old fat PS2 is better. USB is unstable on the slim PSTwo since
version v14.
You need a method to start PS2 homebrew software.
You can control the kernelloader software with the first game pad or
a USB keyboard (UP, DOWN, CROSS or RETURN).
To enter texts (e.g. kernel parameter) you need the USB keyboard.

The easiest way for testing is using the PS2 Linux Live DVD at:
http://sourceforge.net/projects/kernelloader/files/BlackRhino%20Linux%20Distribution/Live%20Linux%20DVD/
The Linux DVD doesn't work on v12/v13 PS2 consoles from some regions.

Otherwise you will need a Linux kernel and a initrd for testing:
http://sourceforge.net/projects/kernelloader/files/Linux%202.4/Linux%202.4.17%20Kernel/
http://sourceforge.net/projects/kernelloader/files/Initial%20RAM%20Disc/

You can copy the files to an USB stick and then select the files in the
menu ("Select Kernel" -> vmlinux, "Select RAM disc" -> initrd).

Virtual Keyboard
################
The input fields in kernelloader supports a virtual keyboard. The virtual
keyboard is controlled by the game pad. This allows to enter kernel
parameter with the game pad without an USB keyboard.
The cursor is blinking. When it blinks it shows for a short time an
underline sign and for the remaining time the character which will
be inserted when pressing triangle. The default is the invisible
space character. The buttons have the following functions:
- The character is selected with R1 and L1.
- The triangle button inserts the character. It can be inserted multiple
  times.
- Left and right moves the cursor left and right.
- The square button deletes the character which is left to the cursor.
- The circle button is for fast jumping in the character selection. This
  is the same as pressing R1 for 16 times. You get from a capital letter
  to lower case by pressing circle 2 times.
- The start button jumps to the beginning of the text.
- The select button jumps to the end of the string.

Video Mode
##########
The application use the default video mode configured for your PS2. The
default mode is PAL, NTSC or 480i when component output is selected
instead of RGB/Composite. To use it with VGA monitor you need the VGA
cable and a monitor which supports SOG (Sync On Green). When kernelloader
is started, you can switch the video mode using the L2 and R2 buttons
on the first pad. After a mode change, you should go to the file menu
and restore defaults, because this will automatically set the kernel
parameter accordingly.
The "+" and "-" keys on the keyboard also switch the video mode when
the focus is not in a input field.
The following function keys switch the screen mode:
  F1: Auto detect
  F2: VGA 640x480 60Hz
  F3: VGA 640x480 72Hz
  F4: VGA 640x480 75Hz
  F5: VGA 640x480 85Hz
  F6: DTV 480P
  F7: NTSC
  F8: PAL

Auto-Boot
#########

To stop Auto-Boot you need to press a button on the first game pad or
a key on the USB keyboard.

Recommended Peripherial Hardware
################################

The following hardware is recommended:
- USB keyboard (need to be supported by Linux 2.4.17)
- USB mouse for use with the graphical user interface
- PS2 memory card (original or a newer version from Datel which works
  without a boot CD)
- PS2 game pad (USB keyboard and mouse also works)
- HDD adapter for the old fat PS2. The adapter can be used to connect
  a parallel IDE hard disc and a network cable. The slim PSTwo has
  already a built-in network adapter. The HDD adapter can't be connected
  to the slim PSTwo. As far as I know there are HDD extensions available
  for the v12. Later slim PStwo consoles doesn't support these extensions.
- USB memory stick to store the Linux kernel and the initrd files. It can
  be used instead of the hard disc to store the file system for Linux. Don't
  forget that USB is unstable on slim PSTwo v14 and higher.
- A readable disc must be in the tray when starting kernelloader (The tray
  must be also closed). Otherwise there can be a freeze until a disc is
  inserted.

If HDD adapter is not connected, kernelloader will not load the network
modules, even if these are enabled.
If you have a HDD adapter or a slim the network cable need to be connected
before starting kernelloader and there must be a network link.

How to Build a VGA cable
########################

You can build your VGA cable yourself without soldering. You need to buy the
following products:
- one PS2 or PS3 component cable (YPbPr)
- one RGB cinch to VGA adaper (1 x female Sub-D 15 pol, 3 x cinch female or
  male with colours
  Red, Green and Blue)
- If the 3 x cinch is male, you need a 3 x cinch adapter

The colours of the RGB VGA adapter are the same on the PS2/PS3 component cable.
Connect the cinch connectors as follows:
Pr (red) to R (red)
Y (green) to G (green)
Pb (blue) to B (blue)

DVD Video
#########

The kernelloader and the newer Linux 2.4.17 supports reading of DVD video.
This can be used to read burned DVDs. The module eromdrvloader.irx enables
the support for this feature. The PS2 Linux Live DVD uses this feature
to be able to run from a DVD without a modchip. To be able to use burned
DVD, the DVD must be conform to the DVD video standard. This
feature will not work on the first PS2. There are also problems on the
slim PSTwo, especically v12. On the v12 the reading of the NVM fails. The
region can't be detected and the wrong module may be loaded. If you report
an error, please include the informations from the Versions menu of
kernelloader.

Configuration Order
###################

At startup the default configuration is set. Then configuration is loaded
from a file. When a value is missing in the configuration file the default
configuration is used. The search order of the configuration files is
"cdfs:config.txt" and then "mc0:kloader/config.txt". The auto boot time and
the video mode is part of the configuration file.

ROMGSCRT
########

The ROMGSCRT module is responsible for setting up the video mode.

SBIOS
#####

The SBIOS is the interface between the Emotion Engine (main processor)
and the IOP (Input/Output processor). Linux runs directly on the Emotion
Engine and uses SBIOS and ROMGSCRT. On the IOP modules from Sony are
running (IRX).

The Great Experiement (TGE)
###########################

Kernelloader is based on TGE from Marcus R. Brown. The TGE includes the
SBIOS and the modules intrelay.irx and dmarelay.irx. dmarelay.irx from
TGE is not working.

Run Time Environment (RTE)
##########################

The RTE is stored on the first disc of the offical Sony Linux Kit. TGE
(The Great Experiement) is normally used by kernelloader. Some stuff
like sound and DMA is not fully working in TGE. You may need stuff from
RTE to get full support.

Sound Support
#############

If your PS2 is an old fat PS2 (v7 or earlier, maybe also v8 or v9, but not
the first PS2 without libsd), sound is automatically working. LIBSD 1.04
or lower is needed to get sound support. There is also the SDRDRV needed.
The newer PS2 consoles include newer versions of LIBSD and SDRDRV. These
versions don't work with the Linux sound driver. To get sound working
you can get these files from RTE, an old game disc or from a BIOS dump
of a old PS2.

DMA Support
###########

DMA is needed for faster transfer rates (Sound and IDE). To get DMA support
you need the module dmarelay.irx from RTE. Linux 2.4.17 doesn't support
this module, you need the old Linux 2.2 from Sony.

Module Information
##################

You can select which IOP modules are loaded in kernelloader. Module names
prefixed with "rom0:X" are the new rom modules. Module names without the
"X" are the old ones. Each playstation has the same version of the old modules.
ps2link is only for debugging. It's purpose in this project is to print debug
messages over network. The messages can be seen on the host using ps2client.
The new modules can be different. Here is more information about the modules
(the modules are listed in groups, only one module of a group is required):

eedebug.irx
Required: No
Patch: patches/linux-2.4.17_ps2-iop-debug.patch
Module send all output of IOP processor to host. Linux patch is required
to see this information at "/proc/ps2iopdebug".

init.irx
imodule1.irx
imodule2.irx
imodule3.irx
imodule4.irx
imodule5.irx
Required: No
You can copy a module to mc0:kloader/ to add a custom module, you
want to load.

SIO2MAN
XSIO2MAN
sio2man.irx
freesio2.irx
Required: Yes
System module. Required to start other modules.

MCMAN
XMCMAN
mcman.irx
Required: Yes, or accessing memroy cards.
Access to PS2 memory cards on IOP.

MCSERV
XMCSERV
mcserv.irx
Required: Yes, or accessing memroy cards.
RPC server for MCMAN. Linux will call the RPC server.

PADMAN
XPADMAN
padman.irx
freepad
Required: Yes, for accessing playstation game controllers.
Driver and RPC server to access gamecontrollers by EE.

iomanX.irx
Required: No, only for ps2link.
File IO driver on IOP.

poweroff.irx
Required: No, only for ps2link.
To get poweroff button working when hard disc is used (DEV9, ps2link).

dev9init.irx
Required: Yes, only for fat PS2. Incompatible with slim PSTwo.
Configure dev9 (expansion bay, ethernet + hdd).

ps2dev9.irx
Required: Yes, for slim PSTwo and for ps2link.
Driver for DEV9 (hardware interface to expansion bay or PCMCIA).
Grants access to network and hard disc.

ps2ip.irx
Reuqired: No, only for ps2link.
TCP/IP network stack on IOP processor.

ps2smap.irx
Required: No, only for ps2link.
Network driver on IOP processor. Linux has it's own driver. When
this driver is used and Linux driver is activated. Linux driver will
deactivate this one. When IOP tries to use the network the system will hang.
This module is incompatible with ps2smap.irx.

smaprpc.irx
Required: Yes, for slim PSTwo if you want to use ethernet.
Network driver on IOP processor. You need also to load ps2dev9.irx.
This module is incompatible with ps2smap.irx.

ps2link.irx
Required: No, only for ps2link.
ps2link is a IOP module which helps debugging.

sharedmem.irx
Required: No, only for ps2link or if you applied the patch.
Patch: patches/linux-2.4.17_ps2-printk.patch
This module is only for debugging and is easier than the RPC interface. Its
purpose is to get a easy way to print messages on EE. If you applied the patch
Linux will only start if you load the module.

iopintr.irx
intrelay-direct.irx
intrelay-direct-rpc.irx
intrelay-dev9.irx
intrelay-dev9-rpc.irx
Required: Yes
Redirects interrupts from IOP to EE.
You need only one module. The normal module is "intrelay-direct.irx". If you
use ps2link or ps2dev9 you need to load "intrelay-dev9.irx". The RTE module
"iopintr.irx" doesn't include the USB initialisation. "intrelay-*.irx" modules
include the USB initialisation, so it is recommended to use "intrelay-*.irx"
instead of the RTE module. When you have a slim PSTwo you should use a module
with "rpc" in the name and a Linux patch:

patches/linux-2.4.17_ps2-rpc-irq.patch

System can hang if you try to use network from IOP and EE!

dmarelay.irx
Required: No
Redirects DMA from EE to IOP and backwards. There is no Linux driver using it
correctly. Its purpose is to speed up network and hard disc. Only the RTE
module is working.

CDVDMAN
XCDVDMAN
cdvdman.irx
Required: Yes
Reading CDs and DVDs.

CDVDFSV
XCDVDFSV
Required: No
I believe it is not required by Linux.

ADDDRV
Required: Yes, for reading video DVDs.
Enables access to rom1.

eromdrvloader
EROMDRV
Required: Yes, for reading video DVDs.
Need to be loaded to read video DVDs.

LIBSD
libsd.irx
freesd.irx
Required: Yes, for sound.
Need to be loaded to get sound working. Only the RTE module is working.
ROM1:/LIBSD from SCPH-77004 is working with sdrdrv.irx from RTE.

SDRDRV
sdrdrv.irx
Required: Yes, for sound with original Sony Linux Kernel.
Sound server. Only the RTE module and ROM1:/SDRDRV from SCPH-39004 is working.
Modules from SCPH-50004 and higher are not working (SDR driver version 4.0.1 (C) SCEI).
The RTE version is: SDR driver version 2.0.0 (C)SCEI
The SCPH-39004 version is: sdr driver version 1.4.0 (C)SCEI

audsrv.irx
Required: Yes, for sound with special Linux Kernel.
Sound server.

ioptrap.irx
Required: No, only for ps2link
Module is only for debugging.

RMMAN
RMMAN2
Required: No
Driver for remote control. This not supported in TGE and RTE SBIOS.

Loading Custom Kernelloader Modules
###################################
Kernelloader includes default modules. You can replace the modules by copying
other versions to the directory "mc0:kloader/". The following modules are loaded
for kernelloader. These modules are only used within kernelloader and not
by Linux. Here is the list in loading order:
SMSUTILS.irx
SMSCDVD.irx
ioptrap.irx
iomanX.irx
poweroff.irx
ps2dev9.irx
ps2ip.irx
ps2smap.irx
ps2link.irx
usbd.irx
usb_mass.irx
fileXio.irx
ps2kbd.irx

When your USB memory stick is not supported within kernelloader, you can
replace usbd.irx and usb_mass.irx. Just copy the files to "mc0:kloader/".
It is possible that kernelloader will not start afterwards. To get it working
again just remove the files or remove the memory card.

Loading Custom Linux Modules
############################
Kernelloader includes default modules. You can replace the modules by copying
other versions to the directory "mc0:kloader/". The following modules are
loaded for Linux. These modules are only used by Linux. Here is the list in
loading order:
init.irx (*)
sio2man.irx
mcman.irx
mcserv.irx
padman.irx
libsd.irx
sdrdrv.irx
iopintr.irx
dmarelay.irx
cdvdman.irx
cdvdfsv.irx
module1.irx (*)
module2.irx (*)
module3.irx (*)
module4.irx (*)
module5.irx (*)

sbios.elf

These modules need to be selected in the module configuration menu. These
modules are automatially choosen if you submit with RTE modules enabled.
There is a menu entry which copies the RTE modules from Sony's Linux kit DVD.
sbios.elf is extracted from pbpx_955.09.
Modules marked with (*) are not part of RTE and can be used to load custom
modules.

Extracting RTE modules and SBIOS
################################
At the end of the configuration menu there is a menu entry for copying RTE
modules and SBIOS from Sony's Linux Kit DVD 1. There are menu entries for
changing the source path if your DVD looks different. When the files doesn't
exist, it is possible that it will hang. This problem is caused by the Video
DVD driver. You need to copy/extract all files.
Then you can select all RTE modules and RTE SBIOS in the module and
configuration menu. You need also to disable CDVD SBIOS calls, because Linux
will not start when the calls are enabled and RTE SBIOS is used. Don't use
iopintr.irx from RTE, because USB will not work. The intrelay.irx from
TGE is working better.
