# Thats Not My Neighbor - PS3 Port
# Makefile for PSL1GHT SDK

.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. export PSL1GHT=<path>")
endif

include $(PSL1GHT)/Makefile.base

TARGET		:= thats-not-my-neighbor
TITLE_ID	:= TNMN00001
APP_NAME	:= That's Not My Neighbor
APP_VER		:= 01.00

BUILD		:= build
SOURCE		:= src
INCLUDE		:= include
DATA		:= assets

CFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.c)))
SFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.s)))

export OFILES := $(CFILES:.c=.o) $(SFILES:.s=.o)
export BINFILES :=

export INCLUDE	+= -I$(CURDIR)/$(INCLUDE) -I$(PSL1GHT)/include -I$(PORTLIBS)/include
export LIBDIRS	+= -L$(CURDIR)/lib -L$(PSL1GHT)/lib -L$(PORTLIBS)/lib
export LIBS		:= -lreality -lsysutil -lio -laudio -lpad -lm -lc

CFLAGS		:= -O2 -Wall -mcpu=cell $(INCLUDE)
CXXFLAGS	:= $(CFLAGS)

export VPATH := $(foreach dir,$(SOURCE),$(CURDIR)/$(dir))

.PHONY: all clean pkg

all: $(BUILD) $(TARGET).self

pkg: $(TARGET).pkg

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

$(TARGET).self: $(BUILD)
	@$(MAKE) -C $(BUILD) -f $(CURDIR)/Makefile.build

$(TARGET).pkg: $(TARGET).self
	@echo "Creating PKG..."
	@mkdir -p pkg/USRDIR
	@cp $(TARGET).self pkg/USRDIR/EBOOT.BIN
	@make_sfo --title "$(APP_NAME)" --appid "$(TITLE_ID)" --version "$(APP_VER)" ps3/PARAM.SFO
	@cp ps3/PARAM.SFO pkg/PARAM.SFO
	@if [ -f ps3/ICON0.PNG ]; then cp ps3/ICON0.PNG pkg/ICON0.PNG; fi
	@if [ -f ps3/PIC1.PNG ]; then cp ps3/PIC1.PNG pkg/PIC1.PNG; fi
	@pkg.py --contentid UP0001-$(TITLE_ID)_00-0000000000000000 pkg/ $(TARGET).pkg
	@echo "Built $(TARGET).pkg successfully!"

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).self $(TARGET).pkg pkg/
