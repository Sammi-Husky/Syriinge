#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	sy_core
RELID		:=  224
BUILD		:=	build
SOURCES		:=	source $(wildcard source/*) lib/BrawlHeaders/PowerPC_EABI_Support/Runtime/Src
INCLUDES	:=	include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CC 			:= $(TOPDIR)/tools/MWCC4_2/mwcceppc.exe
CXX 		:= $(TOPDIR)/tools/MWCC4_2/mwcceppc.exe
LD 			:= $(TOPDIR)/tools/MWCC4_2/mwldeppc.exe
ELF2REL		:= $(TOPDIR)/tools/elf2rel.exe


CCFLAGS		:= -msgstyle gcc -Cpp_exceptions off -c -use_lmw_stmw on -RTTI off -proc gekko -nostdinc -O4,s -enum int -fp hard -u _prolog -u _epilog -u _unresolved -sdata 0 -sdata2 0 -func_align 4 -rostr
CXXFLAGS	:= -lang=c++ $(CCFLAGS)
LDFLAGS		:= -lcf $(LCF) -r1 -fp hard -m _prolog -g

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export TOPDIR	:= $(CURDIR)

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

# For REL linking
export LDFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ld)))
export MAPFILE		:= $(CURDIR)/lib/BrawlHeaders/RSBE01.lst
export LCF			:= $(TOPDIR)/rel.lcf

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			-I$(CURDIR)/$(BUILD) -I- \
			-I$(CURDIR)/lib/PowerPC_EABI_Support/Runtime/Inc \
			-I$(CURDIR)/lib/BrawlHeaders/Brawl/Include \
			-I$(CURDIR)/lib/BrawlHeaders/nw4r/include \
			-I$(CURDIR)/lib/BrawlHeaders/OpenRVL/include/ \
			-I$(CURDIR)/lib/BrawlHeaders/OpenRVL/include/MetroTRK \
			-I$(CURDIR)/lib/BrawlHeaders/OpenRVL/include/revolution \
			-I$(CURDIR)/lib/BrawlHeaders/OpenRVL/include/RVLFaceLib \
			-I$(CURDIR)/lib/BrawlHeaders/OpenRVL/include/stl \
			-I$(CURDIR)/lib/BrawlHeaders/std/Include \
			-I$(CURDIR)/lib/BrawlHeaders/utils/include \

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean_target

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
ifdef ADDR
	py -3 ./convertMap.py $(TARGET).map $(ADDR) $(TARGET)-dolphin.map
endif


#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).rel $(OUTPUT).map $(OUTPUT)-dolphin.map

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).rel: $(OUTPUT).elf $(MAPFILE)
$(OUTPUT).elf: $(LDFILES) $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

# REL linking
%.rel: %.elf
	@echo output ... $(notdir $@)
	$(SILENTCMD)$(ELF2REL) $< -s $(MAPFILE) --rel-id $(RELID)
	
%.elf:
	@echo linking ... $(notdir $@)
	$(SILENTCMD)$(LD) $^ $(LDFLAGS) -Map $(OUTPUT).map -o $@
	

#---------------------------------------------------------------------------------
# object files
#---------------------------------------------------------------------------------
%.o: %.cpp
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)$(CXX) $(CXXFLAGS) -MMD $< $(INCLUDE) -o $(DEPSDIR)/$*.d
	$(SILENTCMD)$(CXX) $(CXXFLAGS) -c $< $(INCLUDE) -o $@

%.o: %.c
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)$(CC) $(CCFLAGS) -MMD $< $(INCLUDE) -o $(DEPSDIR)/$*.d
	$(SILENTCMD)$(CC) $(CCFLAGS) -c $< $(INCLUDE) -o $@

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
