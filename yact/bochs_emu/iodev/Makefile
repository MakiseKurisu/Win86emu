# Copyright (C) 2001  The Bochs Project
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

# Makefile for the iodev component of bochs


.SUFFIXES: .cc

prefix          = /usr/local
exec_prefix     = $(prefix)
srcdir          = .

bindir          = $(exec_prefix)/bin
libdir          = $(exec_prefix)/lib
datarootdir     = $(prefix)/share
mandir          = $(datarootdir)/man
man1dir         = $(mandir)/man1
man5dir         = $(mandir)/man5
docdir          = $(datarootdir)/doc/bochs
sharedir        = $(datarootdir)/bochs
top_builddir    = ..
top_srcdir      = ..

SHELL = /bin/sh



CXX = cl
CXXFLAGS = $(BX_INCDIRS) /nologo /MT /W3 /EHs-c- /DNDEBUG /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS /Ox 

LDFLAGS = 
LIBS = 
RANLIB = echo
PLUGIN_PATH=$(exec_prefix)/lib
top_builddir    = ..
LIBTOOL=$(SHELL) $(top_builddir)/libtool
WIN32_DLL_IMPORT_LIBRARY=../dllexports.a

CDROM_OBJS = cdrom.o
SOUNDLOW_OBJS = soundwin.o
NETLOW_OBJS = eth.o eth_null.o eth_vnet.o eth_win32.o
USBDEV_OBJS = usb_hid.o usb_hub.o usb_msd.o usb_printer.o
SCSI_OBJS = scsi_device.o

BX_INCDIRS = -I.. -I$(srcdir)/.. -I../instrument/stubs -I$(srcdir)/../instrument/stubs
LOCAL_CXXFLAGS = $(MCH_CFLAGS)

PCIDEV_CXXFLAGS = -I$(srcdir)/../host/linux/pcidev

OBJS_THAT_CANNOT_BE_PLUGINS = \
  devices.o \
  virt_timer.o \
  slowdown_timer.o \
  $(MCH_OBJS)

OBJS_THAT_CAN_BE_PLUGINS = \
  pic.o \
  pit_wrap.o \
  serial.o \
  parallel.o \
  floppy.o \
  keyboard.o \
  vga.o \
  biosdev.o \
  cmos.o \
  harddrv.o \
  hdimage.o \
  dma.o \
  unmapped.o \
  extfpuirq.o \
  speaker.o \
  busmouse.o \
  ioapic.o \
  soundmod.o \
  gameport.o \
  pci.o pci2isa.o pci_ide.o pcivga.o acpi.o usb_uhci.o usb_ohci.o \
  usb_common.o \
  sb16.o \
  ne2k.o \
  

OBJS_THAT_SUPPORT_OTHER_PLUGINS = \
  pit82c54.o \
  scancodes.o \
  serial_raw.o \
  svga_cirrus.o \
  vmware3.o \
  vmware4.o \
  vvfat.o \
  $(CDROM_OBJS) \
  $(SOUNDLOW_OBJS) \
  $(NETLOW_OBJS) \
  $(USBDEV_OBJS) \
  $(SCSI_OBJS)

NONPLUGIN_OBJS = $(OBJS_THAT_CANNOT_BE_PLUGINS) $(OBJS_THAT_CAN_BE_PLUGINS) $(OBJS_THAT_SUPPORT_OTHER_PLUGINS)
PLUGIN_OBJS = 
SOUND_LINK_OPTS = 

all: libiodev.a

plugins: $(PLUGIN_OBJS:%.o=libbx_%.la)

libiodev.a: $(NONPLUGIN_OBJS)
	-del libiodev.a
	lib /nologo /subsystem:console /verbose /out:$@ $(NONPLUGIN_OBJS)
	echo libiodev.a

# standard compile rule for C++ files
.cc.o:
	$(CXX) /c  $(CXXFLAGS) $(LOCAL_CXXFLAGS) /Tp$< /Fo$@

pcidev.o : pcidev.cc
	$(CXX) /c  $(CXXFLAGS) $(LOCAL_CXXFLAGS) $(PCIDEV_CXXFLAGS) /Tp$< /Fo$@

##### building plugins with libtool
%.lo: %.cc
	$(LIBTOOL) --mode=compile $(CXX) -c $(CXXFLAGS) $(LOCAL_CXXFLAGS) $< -o $@

pcidev.lo : pcidev.cc
	$(LIBTOOL) --mode=compile $(CXX) -c $(CXXFLAGS) $(LOCAL_CXXFLAGS) $(PCIDEV_CXXFLAGS) $< -o $@

libbx_%.la: %.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH)

# special link rules for plugins that require more than one object file
libbx_harddrv.la: harddrv.lo $(CDROM_OBJS:.o=.lo)
	$(LIBTOOL) --mode=link $(CXX) -module harddrv.lo $(CDROM_OBJS:.o=.lo) -o libbx_harddrv.la -rpath $(PLUGIN_PATH)

libbx_hdimage.la: hdimage.lo vmware3.lo vmware4.lo vvfat.lo
	$(LIBTOOL) --mode=link $(CXX) -module hdimage.lo vmware3.lo vmware4.lo vvfat.lo -o libbx_hdimage.la -rpath $(PLUGIN_PATH)

libbx_keyboard.la: keyboard.lo scancodes.lo
	$(LIBTOOL) --mode=link $(CXX) -module keyboard.lo scancodes.lo -o libbx_keyboard.la -rpath $(PLUGIN_PATH)

libbx_pit_wrap.la: pit82c54.lo pit_wrap.lo
	$(LIBTOOL) --mode=link $(CXX) -module pit82c54.lo pit_wrap.lo -o libbx_pit.la -rpath $(PLUGIN_PATH)

libbx_soundmod.la: soundmod.lo $(SOUNDLOW_OBJS:.o=.lo)
	$(LIBTOOL) --mode=link $(CXX) -module soundmod.lo $(SOUNDLOW_OBJS:.o=.lo) -o libbx_soundmod.la -rpath $(PLUGIN_PATH) $(SOUND_LINK_OPTS)

libbx_ne2k.la: ne2k.lo $(NETLOW_OBJS:.o=.lo)
	$(LIBTOOL) --mode=link $(CXX) -module ne2k.lo $(NETLOW_OBJS:.o=.lo) -o libbx_ne2k.la -rpath $(PLUGIN_PATH)

libbx_pcipnic.la: pcipnic.lo $(NETLOW_OBJS:.o=.lo)
	$(LIBTOOL) --mode=link $(CXX) -module pcipnic.lo $(NETLOW_OBJS:.o=.lo) -o libbx_pcipnic.la -rpath $(PLUGIN_PATH)

libbx_serial.la: serial.lo serial_raw.lo
	$(LIBTOOL) --mode=link $(CXX) -module serial.lo serial_raw.lo -o libbx_serial.la -rpath $(PLUGIN_PATH)

libbx_vga.la: vga.lo svga_cirrus.lo
	$(LIBTOOL) --mode=link $(CXX) -module vga.lo svga_cirrus.lo -o libbx_vga.la -rpath $(PLUGIN_PATH)

libbx_usb_common.la: usb_common.lo $(USBDEV_OBJS:.o=.lo) scsi_device.lo $(CDROM_OBJS:.o=.lo)
	$(LIBTOOL) --mode=link $(CXX) -module usb_common.lo $(USBDEV_OBJS:.o=.lo) scsi_device.lo $(CDROM_OBJS:.o=.lo) -o libbx_usb_common.la -rpath $(PLUGIN_PATH)

#### building DLLs for win32  (tested on cygwin only)
bx_%.dll: %.o
	$(CXX) $(CXXFLAGS) -shared -o $@ $< $(WIN32_DLL_IMPORT_LIBRARY)

# special link rules for plugins that require more than one object file
bx_harddrv.dll: harddrv.o $(CDROM_OBJS)
	$(CXX) $(CXXFLAGS) -shared -o bx_harddrv.dll harddrv.o $(CDROM_OBJS) $(WIN32_DLL_IMPORT_LIBRARY)

bx_hdimage.dll: hdimage.o vmware3.o vmware4.o vvfat.o
	$(CXX) $(CXXFLAGS) -shared -o bx_hdimage.dll hdimage.o vmware3.o vmware4.o vvfat.o $(WIN32_DLL_IMPORT_LIBRARY)

bx_keyboard.dll: keyboard.o scancodes.o
	$(CXX) $(CXXFLAGS) -shared -o bx_keyboard.dll keyboard.o scancodes.o $(WIN32_DLL_IMPORT_LIBRARY)

bx_pit_wrap.dll: pit82c54.o pit_wrap.o
	$(CXX) $(CXXFLAGS) -shared -o bx_pit.dll pit82c54.o pit_wrap.o $(WIN32_DLL_IMPORT_LIBRARY)

bx_soundmod.dll: soundmod.o $(SOUNDLOW_OBJS)
	$(CXX) $(CXXFLAGS) -shared -o bx_soundmod.dll soundmod.o $(SOUNDLOW_OBJS) $(WIN32_DLL_IMPORT_LIBRARY) -lwinmm

bx_ne2k.dll: ne2k.o $(NETLOW_OBJS)
	$(CXX) $(CXXFLAGS) -shared -o bx_ne2k.dll ne2k.o $(NETLOW_OBJS) $(WIN32_DLL_IMPORT_LIBRARY)

bx_pcipnic.dll: pcipnic.o $(NETLOW_OBJS)
	$(CXX) $(CXXFLAGS) -shared -o bx_pcipnic.dll pcipnic.o $(NETLOW_OBJS) $(WIN32_DLL_IMPORT_LIBRARY)

bx_gameport.dll: gameport.o
	$(CXX) $(CXXFLAGS) -shared -o bx_gameport.dll gameport.o $(WIN32_DLL_IMPORT_LIBRARY) -lwinmm

bx_serial.dll: serial.o serial_raw.o
	$(CXX) $(CXXFLAGS) -shared -o bx_serial.dll serial.o serial_raw.o $(WIN32_DLL_IMPORT_LIBRARY) -lwsock32

bx_vga.dll: vga.o svga_cirrus.o
	$(CXX) $(CXXFLAGS) -shared -o bx_vga.dll vga.o svga_cirrus.o $(WIN32_DLL_IMPORT_LIBRARY)

bx_usb_common.dll: usb_common.o $(USBDEV_OBJS) scsi_device.o $(CDROM_OBJS)
	$(CXX) $(CXXFLAGS) -shared -o bx_usb_common.dll usb_common.o $(USBDEV_OBJS) scsi_device.o $(CDROM_OBJS) $(WIN32_DLL_IMPORT_LIBRARY)

##### end DLL section

clean:
	-del -rf .libs *.lo *.o *.la *.a *.dll

dist-clean: clean
	-del Makefile

###########################################
# dependencies generated by
#  gcc -MM -I.. -I../instrument/stubs *.cc | sed -e 's/\.cc/.cc/g'
#  gcc -MM -I.. -I../instrument/stubs *.cc | \
#     sed -e 's/\.cc/.cc/g' -e 's/\.o:/.lo:/g'
#
# This means that every source file is listed twice, once with a .o rule
# and then again with an identical .lo rule.  The .lo rules are used when
# building plugins.
###########################################
acpi.o: acpi.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h acpi.h
biosdev.o: biosdev.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h biosdev.h
busmouse.o: busmouse.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h busmouse.h
cdrom_amigaos.o: cdrom_amigaos.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h scsi_commands.h cdrom.h
cdrom_beos.o: cdrom_beos.cc cdrom_beos.h
cdrom.o: cdrom.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  cdrom.h
cmos.o: cmos.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h cmos.h
crc32.o: crc32.cc crc32.h ../config.h
devices.o: devices.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../iodev/virt_timer.h \
  ../iodev/slowdown_timer.h
dma.o: dma.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h dma.h
eth_arpback.o: eth_arpback.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
eth.o: eth.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_fbsd.o: eth_fbsd.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
eth_linux.o: eth_linux.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_null.o: eth_null.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_packetmaker.o: eth_packetmaker.cc iodev.h ../bochs.h ../config.h \
  ../osdep.h ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
eth_tap.o: eth_tap.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_tuntap.o: eth_tuntap.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_vde.o: eth_vde.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_vnet.o: eth_vnet.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_win32.o: eth_win32.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
extfpuirq.o: extfpuirq.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h extfpuirq.h
floppy.o: floppy.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h floppy.h
gameport.o: gameport.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h gameport.h
guest2host.o: guest2host.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h guest2host.h
harddrv.o: harddrv.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h harddrv.h hdimage.h \
  cdrom.h
hdimage.o: hdimage.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vmware3.h \
  vmware4.h vvfat.h
ioapic.o: ioapic.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ioapic.h
iodebug.o: iodebug.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
keyboard.o: keyboard.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../gui/keymap.h \
  keyboard.h scancodes.h
ne2k.o: ne2k.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h ne2k.h eth.h
parallel.o: parallel.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h parallel.h
pci2isa.o: pci2isa.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h pci2isa.h
pci.o: pci.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h
pcidev.o: pcidev.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
pci_ide.o: pci_ide.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h pci_ide.h
pcipnic.o: pcipnic.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
pcivga.o: pcivga.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h pcivga.h
pic.o: pic.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pic.h
pit82c54.o: pit82c54.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pit82c54.h
pit_wrap.o: pit_wrap.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pit_wrap.h pit82c54.h \
  virt_timer.h speaker.h
sb16.o: sb16.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h sb16.h soundmod.h \
  soundlnx.h soundwin.h soundosx.h
scancodes.o: scancodes.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h scancodes.h
scsi_device.o: scsi_device.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h cdrom.h \
  scsi_device.h
serial.o: serial.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h serial.h
serial_raw.o: serial_raw.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
slowdown_timer.o: slowdown_timer.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h slowdown_timer.h
soundlnx.o: soundlnx.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundlnx.h
soundmod.o: soundmod.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundlnx.h \
  soundosx.h soundwin.h
soundosx.o: soundosx.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundosx.h
soundwin.o: soundwin.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundwin.h
speaker.o: speaker.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h speaker.h
svga_cirrus.o: svga_cirrus.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h vga.h svga_cirrus.h
unmapped.o: unmapped.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h unmapped.h
usb_common.o: usb_common.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_hid.h usb_hub.h usb_msd.h usb_printer.h
usb_hid.o: usb_hid.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_hid.h
usb_hub.o: usb_hub.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_hub.h
usb_msd.o: usb_msd.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h cdrom.h \
  hdimage.h scsi_device.h usb_msd.h
usb_ohci.o: usb_ohci.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h usb_common.h \
  usb_ohci.h
usb_printer.o: usb_printer.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_printer.h
usb_uhci.o: usb_uhci.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h usb_common.h \
  usb_uhci.h
vga.o: vga.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h vga.h
virt_timer.o: virt_timer.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h virt_timer.h
vmware3.o: vmware3.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vmware3.h
vmware4.o: vmware4.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vmware4.h
vvfat.o: vvfat.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vvfat.h
acpi.lo: acpi.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h acpi.h
biosdev.lo: biosdev.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h biosdev.h
busmouse.lo: busmouse.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h busmouse.h
cdrom_amigaos.lo: cdrom_amigaos.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h scsi_commands.h cdrom.h
cdrom_beos.lo: cdrom_beos.cc cdrom_beos.h
cdrom.lo: cdrom.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  cdrom.h
cmos.lo: cmos.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h cmos.h
crc32.lo: crc32.cc crc32.h ../config.h
devices.lo: devices.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../iodev/virt_timer.h \
  ../iodev/slowdown_timer.h
dma.lo: dma.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h dma.h
eth_arpback.lo: eth_arpback.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
eth.lo: eth.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_fbsd.lo: eth_fbsd.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
eth_linux.lo: eth_linux.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_null.lo: eth_null.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_packetmaker.lo: eth_packetmaker.cc iodev.h ../bochs.h ../config.h \
  ../osdep.h ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
eth_tap.lo: eth_tap.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_tuntap.lo: eth_tuntap.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_vde.lo: eth_vde.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_vnet.lo: eth_vnet.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h eth.h
eth_win32.lo: eth_win32.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
extfpuirq.lo: extfpuirq.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h extfpuirq.h
floppy.lo: floppy.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h floppy.h
gameport.lo: gameport.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h gameport.h
guest2host.lo: guest2host.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h guest2host.h
harddrv.lo: harddrv.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h harddrv.h hdimage.h \
  cdrom.h
hdimage.lo: hdimage.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vmware3.h \
  vmware4.h vvfat.h
ioapic.lo: ioapic.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ioapic.h
iodebug.lo: iodebug.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
keyboard.lo: keyboard.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../gui/keymap.h \
  keyboard.h scancodes.h
ne2k.lo: ne2k.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h ne2k.h eth.h
parallel.lo: parallel.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h parallel.h
pci2isa.lo: pci2isa.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h pci2isa.h
pci.lo: pci.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h
pcidev.lo: pcidev.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
pci_ide.lo: pci_ide.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h pci_ide.h
pcipnic.lo: pcipnic.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
pcivga.lo: pcivga.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h pcivga.h
pic.lo: pic.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pic.h
pit82c54.lo: pit82c54.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pit82c54.h
pit_wrap.lo: pit_wrap.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pit_wrap.h pit82c54.h \
  virt_timer.h speaker.h
sb16.lo: sb16.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h sb16.h soundmod.h \
  soundlnx.h soundwin.h soundosx.h
scancodes.lo: scancodes.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h scancodes.h
scsi_device.lo: scsi_device.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h cdrom.h \
  scsi_device.h
serial.lo: serial.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h serial.h
serial_raw.lo: serial_raw.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h
slowdown_timer.lo: slowdown_timer.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h slowdown_timer.h
soundlnx.lo: soundlnx.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundlnx.h
soundmod.lo: soundmod.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundlnx.h \
  soundosx.h soundwin.h
soundosx.lo: soundosx.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundosx.h
soundwin.lo: soundwin.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h soundmod.h soundwin.h
speaker.lo: speaker.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h speaker.h
svga_cirrus.lo: svga_cirrus.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h vga.h svga_cirrus.h
unmapped.lo: unmapped.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h unmapped.h
usb_common.lo: usb_common.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_hid.h usb_hub.h usb_msd.h usb_printer.h
usb_hid.lo: usb_hid.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_hid.h
usb_hub.lo: usb_hub.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_hub.h
usb_msd.lo: usb_msd.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h cdrom.h \
  hdimage.h scsi_device.h usb_msd.h
usb_ohci.lo: usb_ohci.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h usb_common.h \
  usb_ohci.h
usb_printer.lo: usb_printer.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h usb_common.h \
  usb_printer.h
usb_uhci.lo: usb_uhci.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h pci.h usb_common.h \
  usb_uhci.h
vga.lo: vga.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h vga.h
virt_timer.lo: virt_timer.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h virt_timer.h
vmware3.lo: vmware3.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vmware3.h
vmware4.lo: vmware4.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vmware4.h
vvfat.lo: vvfat.cc iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h hdimage.h vvfat.h
