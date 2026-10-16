# Win86emun Cross-Compilation Makefile for MinGW-w64
# Target: Windows x86 (i686)

CROSS_PREFIX = i686-w64-mingw32
CXX = $(CROSS_PREFIX)-g++
CC = $(CROSS_PREFIX)-gcc
WINDRES = $(CROSS_PREFIX)-windres

CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fno-strict-aliasing
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -shared -static-libgcc -static-libstdc++

INCLUDES = -I./yact/inc -I./yact
DEFINES = -DWIN32 -D_WINDOWS -D_USRDLL -DNDEBUG

YACT_EMU_DIR = yact/yact_emu
X86_EMU_DIR = yact/x86_emu
SUPP_DIR = yact/supp
RELEASE_DIR = release

$(shell mkdir -p $(RELEASE_DIR))

SUPP_OBJS = $(SUPP_DIR)/dllmain.o $(SUPP_DIR)/logging.o $(SUPP_DIR)/supp.o
YACT_EMU_OBJS = $(YACT_EMU_DIR)/cmdstream.o $(YACT_EMU_DIR)/dllmain.o $(YACT_EMU_DIR)/operands.o $(YACT_EMU_DIR)/onebytecmds.o $(YACT_EMU_DIR)/stdafx.o $(YACT_EMU_DIR)/twobytecmds.o $(YACT_EMU_DIR)/yact_emu.o
X86_EMU_OBJS = $(X86_EMU_DIR)/dllmain.o $(X86_EMU_DIR)/stdafx.o $(X86_EMU_DIR)/x86_emu.o

all: $(RELEASE_DIR)/supp.nt.dll $(RELEASE_DIR)/yact_emu.dll $(RELEASE_DIR)/x86_emu.dll

$(RELEASE_DIR)/supp.nt.dll: $(SUPP_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -lkernel32 -luser32 -ladvapi32

$(RELEASE_DIR)/yact_emu.dll: $(YACT_EMU_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -lkernel32

$(RELEASE_DIR)/x86_emu.dll: $(X86_EMU_OBJS) $(RELEASE_DIR)/supp.nt.dll
	$(CXX) $(LDFLAGS) -o $@ $^ -lkernel32 -luser32

$(SUPP_DIR)/%.o: $(SUPP_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -DSUPP_EXPORTS $(INCLUDES) -c $< -o $@

$(YACT_EMU_DIR)/%.o: $(YACT_EMU_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -DYACT_EMU_EXPORTS $(INCLUDES) -c $< -o $@

$(X86_EMU_DIR)/%.o: $(X86_EMU_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -DX86_EMU_EXPORTS $(INCLUDES) -c $< -o $@

clean:
	rm -f $(SUPP_DIR)/*.o $(YACT_EMU_DIR)/*.o $(X86_EMU_DIR)/*.o
	rm -f $(RELEASE_DIR)/*.dll

.PHONY: all clean
