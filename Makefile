# That's Not My Neighbor - PlayStation 3 Port
# Native PSL1GHT v2 + RSX Tiny3D & Embedded Lua 5.4 Demake Engine

TARGET		:= thats-not-my-neighbor
TITLE_ID	:= TNMN00001
APP_NAME	:= That's Not My Neighbor PS3
APP_VER		:= 01.00

# PSL1GHT v2 Toolchain & Environment Settings
PS3DEV		?= /usr/local/ps3dev
PORTLIBS	?= $(PS3DEV)/portlibs/ppu
PSL1GHT		?= $(PS3DEV)

CC			:= ppu-gcc
CXX			:= ppu-g++
MAKE_SELF	:= make_self
PKG_PY		:= pkg.py
INCLUDES	:= -Isrc -Isrc/lua -I$(PS3DEV)/include -I$(PORTLIBS)/include -I$(PS3DEV)/ppu/include
LIBDIRS		:= -L$(PS3DEV)/lib -L$(PORTLIBS)/lib -L$(PS3DEV)/ppu/lib
TINY3D_LIB	:= $(shell for p in $(PORTLIBS)/lib $(PS3DEV)/lib $(PS3DEV)/ppu/lib; do if [ -f $$p/libtiny3d.a ]; then echo "-ltiny3d"; break; fi; done)
LIBS		:= $(TINY3D_LIB) -lgcm_sys -lio -laudio -lsysutil -lm -lc

CFLAGS		:= -O2 -Wall -mcpu=cell -D_PS3 -DPSL1GHT -DLUA_COMPAT_5_3 $(INCLUDES)

# Source Discovery: C Engine + Vanilla Embedded Lua 5.4 Core (excluding standalone CLIs and desktop runner)
SRCS_ENGINE	:= $(filter-out src/main_desktop.c, $(wildcard src/*.c))
SRCS_LUA	:= $(filter-out src/lua/lua.c src/lua/luac.c, $(wildcard src/lua/*.c))
SRCS		:= $(SRCS_ENGINE) $(SRCS_LUA)
OBJS		:= $(SRCS:.c=.o)

# Local PC Desktop Target (SDL2 + Embedded Lua 5.4 + Vorbis)
DESKTOP_TARGET := $(TARGET)-desktop
HOST_CC        := gcc
SDL_CFLAGS     := $(shell sdl2-config --cflags)
SDL_LIBS       := $(shell sdl2-config --libs) -lSDL2_test -lvorbisfile -lvorbis -logg -lm

.PHONY: all clean pkg install desktop run-desktop

all: $(TARGET).self

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	@echo "[LINK] Linking $(TARGET).elf with PSL1GHT v2 toolchain..."
	$(CC) $(OBJS) $(LIBDIRS) $(LIBS) -o $@

$(TARGET).self: $(TARGET).elf
	@echo "[SELF] Generating $(TARGET).self (NPDRM / EBOOT)..."
	$(MAKE_SELF) $< $@

# Production PS3 Package Assembly Target
pkg: $(TARGET).pkg

$(TARGET).pkg: $(TARGET).self
	@echo "[PKG] Assembling PS3 NPDRM directory structure..."
	@rm -rf pkg/
	@mkdir -p pkg/USRDIR/scripts pkg/USRDIR/assets
	@cp $(TARGET).self pkg/USRDIR/EBOOT.BIN
	@cp -r usrdir/assets/* pkg/USRDIR/assets/
	@cp -r usrdir/scripts/* pkg/USRDIR/scripts/
	@if [ -f ps3/PARAM.SFO ]; then cp ps3/PARAM.SFO pkg/PARAM.SFO; fi
	@if [ -f ps3/ICON0.PNG ]; then cp ps3/ICON0.PNG pkg/ICON0.PNG; fi
	@if [ -f ps3/PIC1.PNG ]; then cp ps3/PIC1.PNG pkg/PIC1.PNG; fi
	@echo "[PKG] Building final installable .pkg..."
	@if which pkg.py >/dev/null 2>&1; then \
		pkg.py --contentid UP0001-$(TITLE_ID)_00-0000000000000000 pkg/ $(TARGET).pkg; \
	elif which make_package_npdrm >/dev/null 2>&1 && [ -f ps3/package.conf ]; then \
		make_package_npdrm ps3/package.conf pkg/; \
	elif which package_finalize >/dev/null 2>&1; then \
		package_finalize pkg/ $(TARGET).pkg; \
	else \
		echo "Warning: No pkg tool found in PATH (pkg.py, make_package_npdrm, package_finalize)."; \
		echo "USRDIR structure created in pkg/ for manual or external packaging."; \
	fi
	@echo "[PKG] Build finished successfully."

# Local PC Desktop Runner Targets
desktop: $(DESKTOP_TARGET)

$(DESKTOP_TARGET): src/main_desktop.c $(SRCS_LUA)
	@echo "[HOST_CC] Compiling PC desktop target with SDL2..."
	$(HOST_CC) -O2 -Wall -Isrc -Isrc/lua $(SDL_CFLAGS) $^ $(SDL_LIBS) -o $@
	@echo "[HOST_CC] PC target successfully built: ./$(DESKTOP_TARGET)"

run-desktop: desktop
	./$(DESKTOP_TARGET)

clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(OBJS) $(TARGET).elf $(TARGET).self $(TARGET).pkg pkg/ $(DESKTOP_TARGET)
