#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

include $(DEVKITPRO)/devkitARM/ds_rules

#---------------------------------------------------------------------------------
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	source

ARCH	:=	-mthumb -mthumb-interwork
CFLAGS	:=	-g -Wall -Wno-deprecated-declarations -O2 \
			-mcpu=arm9tdmi -mtune=arm9tdmi -fomit-frame-pointer \
			-ffast-math $(ARCH)
CFLAGS	+=	-I$(CURDIR)/$(SOURCES) \
			-I$(LIBNDS)/include \
			-DARM9
CXXFLAGS := $(CFLAGS)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
# Outer invocation — CURDIR is the libfb directory
#---------------------------------------------------------------------------------
LIBOUT		:=	$(CURDIR)/lib/libfb.a

export VPATH	:= $(CURDIR)/$(SOURCES)
export DEPSDIR	:= $(CURDIR)/$(BUILD)
export LIBOUT
export CFLAGS
export CXXFLAGS
export ASFLAGS

CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@[ -d lib ] || mkdir -p lib
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) lib/libfb.a

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
# Inner invocation — CURDIR is the build subdirectory, LIBOUT is imported
#---------------------------------------------------------------------------------
DEPENDS := $(OFILES:.o=.d)

$(LIBOUT): $(OFILES)
	@echo building libfb.a ...
	@$(AR) rcs $@ $(OFILES)

-include $(DEPENDS)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
