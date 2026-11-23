# Thats Not My Neighbor - PS3 Port
# Direct Standalone Makefile for ppu-gcc

CC		:= ppu-gcc
CXX		:= ppu-g++
MAKE_SELF	:= make_self
MAKE_SFO	:= make_sfo
PKG_PY		:= pkg.py

TARGET		:= thats-not-my-neighbor
TITLE_ID	:= TNMN00001
APP_NAME	:= That's Not My Neighbor
APP_VER		:= 01.00

PS3DEV		?= /usr/local/ps3dev
PORTLIBS	?= $(PS3DEV)/portlibs/ppu

INCLUDES	:= -Iinclude -I$(PS3DEV)/include -I$(PORTLIBS)/include -I$(PS3DEV)/ppu/include
LIBDIRS		:= -L$(PS3DEV)/lib -L$(PORTLIBS)/lib -L$(PS3DEV)/ppu/lib
LIBS		:= -lsysutil -lio -laudio -lm -lc

CFLAGS		:= -O2 -Wall -mcpu=cell $(INCLUDES)
SRC_FILES	:= $(wildcard src/*.c)

.PHONY: all clean pkg

all: $(TARGET).self

$(TARGET).elf: $(SRC_FILES)
	@echo "Compiling $(TARGET).elf with $(CC)..."
	$(CC) $(CFLAGS) $(SRC_FILES) $(LIBDIRS) $(LIBS) -o $@

$(TARGET).self: $(TARGET).elf
	@echo "Creating $(TARGET).self..."
	$(MAKE_SELF) $< $@

pkg: $(TARGET).pkg

$(TARGET).pkg: $(TARGET).self
	@echo "Creating PKG..."
	@mkdir -p pkg/USRDIR
	@cp $(TARGET).self pkg/USRDIR/EBOOT.BIN
	@cp -r assets pkg/USRDIR/
	@$(MAKE_SFO) --title "$(APP_NAME)" --appid "$(TITLE_ID)" --version "$(APP_VER)" ps3/PARAM.SFO
	@cp ps3/PARAM.SFO pkg/PARAM.SFO
	@if [ -f ps3/ICON0.PNG ]; then cp ps3/ICON0.PNG pkg/ICON0.PNG; fi
	@if [ -f ps3/PIC1.PNG ]; then cp ps3/PIC1.PNG pkg/PIC1.PNG; fi
	@$(PKG_PY) --contentid UP0001-$(TITLE_ID)_00-0000000000000000 pkg/ $(TARGET).pkg
	@echo "Built $(TARGET).pkg successfully!"

clean:
	@rm -rf $(TARGET).elf $(TARGET).self $(TARGET).pkg pkg/ ps3/PARAM.SFO
