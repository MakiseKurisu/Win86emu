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

# Makefile for the gui component of bochs


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
LOCAL_CXXFLAGS = 
LDFLAGS = 
LIBS = 
X_CFLAGS = 
RANLIB = echo
PLUGIN_PATH=$(exec_prefix)/lib
top_builddir    = ..
LIBTOOL=$(SHELL) $(top_builddir)/libtool
WIN32_DLL_IMPORT_LIBRARY=../dllexports.a
BX_INCDIRS = -I.. -I$(srcdir)/.. -I../iodev -I$(srcdir)/../iodev -I../instrument/stubs -I$(srcdir)/../instrument/stubs

GUI_OBJS_X11  = x.o
GUI_OBJS_SDL = sdl.o
GUI_OBJS_SVGA = svga.o
GUI_OBJS_BEOS = beos.o
GUI_OBJS_WIN32 = win32.o
GUI_OBJS_MACOS = macintosh.o
GUI_OBJS_CARBON = carbon.o
GUI_OBJS_NOGUI = nogui.o
GUI_OBJS_TERM  = term.o
GUI_OBJS_RFB = rfb.o
GUI_OBJS_AMIGAOS = amigaos.o
GUI_OBJS_WX = wx.o
GUI_OBJS_WX_SUPPORT = wxmain.o wxdialog.o
OBJS_THAT_CANNOT_BE_PLUGINS = keymap.o gui.o siminterface.o paramtree.o textconfig.o enh_dbg.o  win32dialog.o win32paramdlg.o
OBJS_THAT_CAN_BE_PLUGINS = 

X_LIBS = 
X_PRE_LIBS = 
XPM_LIB = 
GUI_LINK_OPTS_X = $(X_LIBS) $(X_PRE_LIBS) -lX11 $(XPM_LIB) -lXrandr
GUI_LINK_OPTS_SDL = `sdl-config --cflags --libs`
GUI_LINK_OPTS_SVGA =  -lvga -lvgagl
GUI_LINK_OPTS_BEOS = -lbe
GUI_LINK_OPTS_RFB =  wsock32.lib
GUI_LINK_OPTS_AMIGAOS =
GUI_LINK_OPTS_WIN32 = -luser32 -lgdi32 -lcomdlg32 -lcomctl32
GUI_LINK_OPTS_WIN32_VCPP = user32.lib gdi32.lib winmm.lib \
  comdlg32.lib comctl32.lib wsock32.lib
GUI_LINK_OPTS_MACOS =
GUI_LINK_OPTS_CARBON = -framework Carbon
GUI_LINK_OPTS_NOGUI =
GUI_LINK_OPTS_TERM = 
GUI_LINK_OPTS_WX = 
GUI_LINK_OPTS =  $(GUI_LINK_OPTS_RFB) $(GUI_LINK_OPTS_WIN32_VCPP)   wsock32.lib winmm.lib

NONPLUGIN_OBJS = $(OBJS_THAT_CANNOT_BE_PLUGINS) $(OBJS_THAT_CAN_BE_PLUGINS) $(OBJS_THAT_SUPPORT_OTHER_PLUGINS)  $(GUI_OBJS_RFB) $(GUI_OBJS_WIN32) $(GUI_OBJS_NOGUI) 
PLUGIN_OBJS = 

#
# -------- end configurable options --------------------------
#

all: libgui.a

plugins: $(PLUGIN_OBJS:%.o=libbx_%.la)

libgui.a: $(NONPLUGIN_OBJS)
	-del libgui.a
	lib /nologo /subsystem:console /verbose /out:$@ $(NONPLUGIN_OBJS)
	echo libgui.a

# standard compile rule for C++ files
.cc.o:
	$(CXX) /c  $(CXXFLAGS) $(LOCAL_CXXFLAGS) /Tp$< /Fo$@

##### building plugins with libtool
%.lo: %.cc
	$(LIBTOOL) --mode=compile $(CXX) -c $(CXXFLAGS) $(LOCAL_CXXFLAGS) $< -o $@

libbx_%.la: %.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH)

libbx_x.la: x.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_X)

libbx_sdl.la: sdl.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_SDL)

libbx_svga.la: svga.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_SVGA)

libbx_beos.la: beos.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_BEOS)

libbx_rfb.la: rfb.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_RFB)

libbx_amigaos.la: amigaos.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_AMIGAOS)

libbx_win32.la: win32.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_WIN32)

libbx_macos.la: macos.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_MACOS)

libbx_carbon.la: carbon.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_CARBON)

libbx_nogui.la: nogui.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_NOGUI)

libbx_term.la: term.lo
	$(LIBTOOL) --mode=link $(CXX) -module $< -o $@ -rpath $(PLUGIN_PATH) $(GUI_LINK_OPTS_TERM)

# special link rules for plugins that require more than one object file
libbx_wx.la: $(GUI_OBJS_WX:.o=.lo) $(GUI_OBJS_WX_SUPPORT:.o=.lo)
	$(LIBTOOL) --mode=link $(CXX) -module $(GUI_OBJS_WX:.o=.lo) $(GUI_OBJS_WX_SUPPORT:.o=.lo) -o libbx_wx.la -rpath $(PLUGIN_PATH)  $(GUI_LINK_OPTS_WX)

#### building DLLs for win32  (tested on cygwin only)
bx_%.dll: %.o
	$(CXX) $(CXXFLAGS) -shared -o $@ $< $(WIN32_DLL_IMPORT_LIBRARY)  $(GUI_LINK_OPTS_WIN32)

bx_wx.dll: $(GUI_OBJS_WX) $(GUI_OBJS_WX_SUPPORT)
	$(CXX) $(CXXFLAGS) -shared -o bx_wx.dll $(GUI_OBJS_WX) $(GUI_OBJS_WX_SUPPORT) $(WIN32_DLL_IMPORT_LIBRARY) `wx-config --libs` -luser32 -lgdi32 -lcomdlg32 -lcomctl32

bx_sdl.dll: $(GUI_OBJS_SDL)
	$(CXX) $(CXXFLAGS) -shared -o bx_sdl.dll $(GUI_OBJS_SDL) $(WIN32_DLL_IMPORT_LIBRARY) $(GUI_LINK_OPTS_SDL)

bx_rfb.dll: $(GUI_OBJS_RFB)
	$(CXX) $(CXXFLAGS) -shared -o bx_rfb.dll $(GUI_OBJS_RFB) $(WIN32_DLL_IMPORT_LIBRARY) $(GUI_LINK_OPTS_RFB)

# no need to build DLLs for beos.o
# no need to build DLLs for x.o

##### end DLL section

clean:
	-del -rf .libs *.la *.a *.lo *.o *.dll

dist-clean: clean
	-del Makefile

###########################################
# all other dependencies generated by
#  gcc -MM -I.. -I../iodev -I../instrument/stubs `wx-config --cxxflags` *.cc | \
#     sed -e 's/\.cc/.cc/g'
#  gcc -MM -I.. -I../iodev -I../instrument/stubs `wx-config --cxxflags` *.cc | \
#     sed -e 's/\.cc/.cc/g' -e 's/\.o:/.lo:/g'
#
# This means that every source file is listed twice, once with a .o rule
# and then again with an identical .lo rule.  The .lo rules are used when
# building plugins.
###########################################
amigaos.o: amigaos.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../iodev/iodev.h
beos.o: beos.cc ../param_names.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../iodev/iodev.h
carbon.o: carbon.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  keymap.h ../iodev/iodev.h ../param_names.h
enh_dbg.o: enh_dbg.cc ../config.h
gtk_enh_dbg_osdep.o: gtk_enh_dbg_osdep.cc ../config.h
gui.o: gui.cc ../iodev/iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../iodev/vga.h \
  keymap.h ../gui/bitmaps/floppya.h ../gui/bitmaps/floppyb.h \
  ../gui/bitmaps/mouse.h ../gui/bitmaps/reset.h ../gui/bitmaps/power.h \
  ../gui/bitmaps/snapshot.h ../gui/bitmaps/copy.h ../gui/bitmaps/paste.h \
  ../gui/bitmaps/configbutton.h ../gui/bitmaps/cdromd.h \
  ../gui/bitmaps/userbutton.h ../gui/bitmaps/saverestore.h
keymap.o: keymap.cc ../param_names.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h keymap.h
macintosh.o: macintosh.cc ../param_names.h ../bochs.h ../config.h \
  ../osdep.h ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../iodev/iodev.h
nogui.o: nogui.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h icon_bochs.h
paramtree.o: paramtree.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h paramtree.h
rfb.o: rfb.cc ../param_names.h ../iodev/iodev.h ../bochs.h ../config.h \
  ../osdep.h ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h keymap.h icon_bochs.h \
  ../font/vga.bitmap.h sdl.h rfb.h rfbkeys.h
sdl.o: sdl.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h icon_bochs.h sdl.h sdlkeys.h
siminterface.o: siminterface.cc ../param_names.h ../iodev/iodev.h \
  ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h ../config.h \
  ../osdep.h ../bxversion.h ../gui/siminterface.h ../gui/paramtree.h \
  ../memory/memory.h ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h \
  ../gui/gui.h ../instrument/stubs/instrument.h
svga.o: svga.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h ../iodev/iodev.h
term.o: term.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h ../iodev/iodev.h
textconfig.o: textconfig.cc ../config.h ../osdep.h ../param_names.h \
  textconfig.h siminterface.h paramtree.h ../extplugin.h ../ltdl.h
win32.o: win32.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h ../iodev/vga.h
win32dialog.o: win32dialog.cc win32dialog.h ../config.h
win32_enh_dbg_osdep.o: win32_enh_dbg_osdep.cc ../config.h
win32paramdlg.o: win32paramdlg.cc win32dialog.h ../config.h
wx.o: wx.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h \
  ../font/vga.bitmap.h wxmain.h
wxdialog.o: wxdialog.cc ../config.h ../param_names.h \
  ../osdep.h ../gui/siminterface.h \
  ../gui/paramtree.h ../bxversion.h wxdialog.h wxmain.h
wxmain.o: wxmain.cc ../config.h ../param_names.h \
  ../osdep.h ../gui/siminterface.h ../gui/paramtree.h ../bxversion.h \
  wxdialog.h wxmain.h ../extplugin.h ../ltdl.h bitmaps/cdromd.xpm \
  bitmaps/copy.xpm bitmaps/floppya.xpm bitmaps/floppyb.xpm bitmaps/paste.xpm \
  bitmaps/power.xpm bitmaps/reset.xpm bitmaps/snapshot.xpm bitmaps/mouse.xpm \
  bitmaps/userbutton.xpm bitmaps/saverestore.xpm icon_bochs.xpm
x.o: x.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h enh_dbg.h icon_bochs.xpm \
  ../font/vga.bitmap.h
amigaos.lo: amigaos.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../iodev/iodev.h
beos.lo: beos.cc ../param_names.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../iodev/iodev.h
carbon.lo: carbon.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  keymap.h ../iodev/iodev.h ../param_names.h
enh_dbg.lo: enh_dbg.cc ../config.h
gtk_enh_dbg_osdep.lo: gtk_enh_dbg_osdep.cc ../config.h
gui.lo: gui.cc ../iodev/iodev.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../param_names.h ../iodev/vga.h \
  keymap.h ../gui/bitmaps/floppya.h ../gui/bitmaps/floppyb.h \
  ../gui/bitmaps/mouse.h ../gui/bitmaps/reset.h ../gui/bitmaps/power.h \
  ../gui/bitmaps/snapshot.h ../gui/bitmaps/copy.h ../gui/bitmaps/paste.h \
  ../gui/bitmaps/configbutton.h ../gui/bitmaps/cdromd.h \
  ../gui/bitmaps/userbutton.h ../gui/bitmaps/saverestore.h
keymap.lo: keymap.cc ../param_names.h ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h keymap.h
macintosh.lo: macintosh.cc ../param_names.h ../bochs.h ../config.h \
  ../osdep.h ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h ../iodev/iodev.h
nogui.lo: nogui.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h icon_bochs.h
paramtree.lo: paramtree.cc ../bochs.h ../config.h ../osdep.h \
  ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h paramtree.h
rfb.lo: rfb.cc ../param_names.h ../iodev/iodev.h ../bochs.h ../config.h \
  ../osdep.h ../bx_debug/debug.h ../config.h ../osdep.h ../bxversion.h \
  ../gui/siminterface.h ../gui/paramtree.h ../memory/memory.h \
  ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h ../gui/gui.h \
  ../instrument/stubs/instrument.h keymap.h icon_bochs.h \
  ../font/vga.bitmap.h sdl.h rfb.h rfbkeys.h
sdl.lo: sdl.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h icon_bochs.h sdl.h sdlkeys.h
siminterface.lo: siminterface.cc ../param_names.h ../iodev/iodev.h \
  ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h ../config.h \
  ../osdep.h ../bxversion.h ../gui/siminterface.h ../gui/paramtree.h \
  ../memory/memory.h ../pc_system.h ../plugin.h ../extplugin.h ../ltdl.h \
  ../gui/gui.h ../instrument/stubs/instrument.h
svga.lo: svga.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h ../iodev/iodev.h
term.lo: term.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h ../iodev/iodev.h
textconfig.lo: textconfig.cc ../config.h ../osdep.h ../param_names.h \
  textconfig.h siminterface.h paramtree.h ../extplugin.h ../ltdl.h
win32.lo: win32.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h
win32dialog.lo: win32dialog.cc win32dialog.h ../config.h
win32_enh_dbg_osdep.lo: win32_enh_dbg_osdep.cc ../config.h
win32paramdlg.lo: win32paramdlg.cc win32dialog.h ../config.h
wx.lo: wx.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h \
  ../font/vga.bitmap.h wxmain.h
wxdialog.lo: wxdialog.cc ../config.h ../param_names.h \
  ../osdep.h ../gui/siminterface.h \
  ../gui/paramtree.h ../bxversion.h wxdialog.h wxmain.h
wxmain.lo: wxmain.cc ../config.h ../param_names.h \
  ../osdep.h ../gui/siminterface.h ../gui/paramtree.h ../bxversion.h \
  wxdialog.h wxmain.h ../extplugin.h ../ltdl.h bitmaps/cdromd.xpm \
  bitmaps/copy.xpm bitmaps/floppya.xpm bitmaps/floppyb.xpm bitmaps/paste.xpm \
  bitmaps/power.xpm bitmaps/reset.xpm bitmaps/snapshot.xpm bitmaps/mouse.xpm \
  bitmaps/userbutton.xpm bitmaps/saverestore.xpm icon_bochs.xpm
x.lo: x.cc ../bochs.h ../config.h ../osdep.h ../bx_debug/debug.h \
  ../config.h ../osdep.h ../bxversion.h ../gui/siminterface.h \
  ../gui/paramtree.h ../memory/memory.h ../pc_system.h ../plugin.h \
  ../extplugin.h ../ltdl.h ../gui/gui.h ../instrument/stubs/instrument.h \
  ../param_names.h keymap.h ../iodev/iodev.h enh_dbg.h icon_bochs.xpm \
  ../font/vga.bitmap.h
