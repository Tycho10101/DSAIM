#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

include $(DEVKITPRO)/devkitARM/ds_rules

#---------------------------------------------------------------------------------
BUILD		:=	build
SOURCES		:=	.
INCLUDES	:=	.

ARCH	:=	-mthumb -mthumb-interwork
CFLAGS	:=	-g -Wall -Wno-deprecated-declarations -O2 \
			-mcpu=arm9tdmi -mtune=arm9tdmi -fomit-frame-pointer \
			-ffast-math $(ARCH)
CFLAGS	+=	-I$(CURDIR) \
			-I$(LIBNDS)/include \
			-DARM9
CXXFLAGS := $(CFLAGS)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
LIBOUT		:=	$(CURDIR)/lib/libTOC.a

export VPATH	:= $(CURDIR)
export DEPSDIR	:= $(CURDIR)/$(BUILD)
export LIBOUT
export CFLAGS
export CXXFLAGS
export ASFLAGS

CFILES		:= $(notdir $(wildcard *.c))
CPPFILES	:= $(notdir $(wildcard *.cpp))

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@[ -d lib ] || mkdir -p lib
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) lib/libTOC.a

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
DEPENDS := $(OFILES:.o=.d)

$(LIBOUT): $(OFILES)
	@echo building libTOC.a ...
	@$(AR) rcs $@ $(OFILES)

-include $(DEPENDS)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
