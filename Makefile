# The name of your project (used to name the compiled .hex file)
TARGET = $(notdir $(CURDIR))

# The teensy version to use, 30 or 31
TEENSY = 31

# Set to 24000000, 48000000, or 96000000 to set CPU core speed
TEENSY_CORE_SPEED = 96000000

# Some libraries will require this to be defined
# If you define this, you will break the default main.cpp
#ARDUINO = 105

# configurable options
OPTIONS = -DLAYOUT_US_ENGLISH -DUSING_MAKEFILE -DUSB_HID

# directory to build in
BUILDDIR = $(abspath $(CURDIR)/build)

#************************************************************************
# Location of Teensyduino utilities, Toolchain, and Arduino Libraries.
# To use this makefile without Arduino, copy the resources from these
# locations and edit the pathnames.  The rest of Arduino is not needed.
#************************************************************************

# path location for Teensy 3 core
COREPATH = cores/teensy3

# path location for Arduino libraries
LIBRARYPATH = /usr/local/Cellar/gcc-arm-none-eabi-49/20150609/arm-none-eabi/include/c++

# path location for the arm-none-eabi compiler
COMPILERPATH = /usr/local/bin

#************************************************************************
# Settings below this point usually do not need to be edited
#************************************************************************

# CPPFLAGS = compiler options for C and C++
CPPFLAGS = -s -Wall -g -Os -mcpu=cortex-m4 -mthumb -MMD $(OPTIONS) -DF_CPU=$(TEENSY_CORE_SPEED) -Isrc -I$(COREPATH)

# compiler options for C++ only
CXXFLAGS = -s -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti

# compiler options for C only
CFLAGS =

# compiler options specific to teensy version
ifeq ($(TEENSY), 31)
    CPPFLAGS += -D__MK20DX256__
    LDSCRIPT = $(COREPATH)/mk20dx256.ld
	TEENSY_CORE_SPEED = 96000000
    UPLOAD_MMCU = mk20dx256
else
	ifeq ($(TEENSY), LC)
    	CPPFLAGS += -D__MKL26Z64__
    	LDSCRIPT = $(COREPATH)/mkl26z64.ld
		TEENSY_CORE_SPEED = 48000000
    	UPLOAD_MMCU = mkl26z64
    else
    	$(error Invalid setting for TEENSY)
    endif
endif

# set arduino define if given
ifdef ARDUINO
	CPPFLAGS += -DARDUINO=$(ARDUINO)
endif


# linker options
#
# WARN:(?) Changed --specs=nano.specs to --specs=nosys.specs to stop link errors
#
LDFLAGS = -Os -Wl,--gc-sections,--defsym=__rtc_localtime=0 --specs=nosys.specs -mcpu=cortex-m4 -mthumb -T$(LDSCRIPT)

# additional libraries to link
LIBS = -lstdc++_nano

# names for the compiler programs
CC = $(abspath $(COMPILERPATH))/arm-none-eabi-gcc
CXX = $(abspath $(COMPILERPATH))/arm-none-eabi-g++
OBJCOPY = $(abspath $(COMPILERPATH))/arm-none-eabi-objcopy
SIZE = $(abspath $(COMPILERPATH))/arm-none-eabi-size
STRIP = $(abspath $(COMPILERPATH))/arm-none-eabi-strip

# automatically create lists of the sources and objects
#LC_FILES := $(wildcard $(LIBRARYPATH)/*/*.c $(LIBRARYPATH)/*/utility/*.c)
#LCPP_FILES := $(wildcard $(LIBRARYPATH)/*/*.cpp $(LIBRARYPATH)/*/utility/*.cpp)
TC_FILES := $(wildcard $(COREPATH)/*.c)
TCPP_FILES := $(filter-out $(COREPATH)/main.cpp, $(wildcard $(COREPATH)/*.cpp))
C_FILES := $(wildcard src/*.c)
CPP_FILES := $(wildcard src/*.cpp)
INO_FILES := $(wildcard src/*.ino)

# include paths for libraries
L_INC := $(foreach lib,$(filter %/, $(wildcard $(LIBRARYPATH)/*/ $(LIBRARYPATH)/*/utility/)), -I$(lib))

SOURCES := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o) $(INO_FILES:.ino=.o) $(TC_FILES:.c=.o) $(TCPP_FILES:.cpp=.o) $(LC_FILES:.c=.o) $(LCPP_FILES:.cpp=.o)
OBJS := $(foreach src,$(SOURCES), $(BUILDDIR)/$(src))

all: $(TARGET).elf

build: $(TARGET).elf

post_compile: $(TARGET).hex
	/usr/local/bin/teensy_loader_cli -mmcu=$(UPLOAD_MMCU) -w -v $(TARGET).hex

# post_compile: $(TARGET).hex
# 	@$(abspath $(TOOLSPATH))/teensy_post_compile -file="$(basename $<)" -path=$(CURDIR) -tools="$(abspath $(TOOLSPATH))"

# reboot:
# 	@-$(abspath $(TOOLSPATH))/teensy_reboot

upload: post_compile

$(BUILDDIR)/%.o: %.c
	@echo "[CC]\t$<"
	@mkdir -p "$(dir $@)"
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(L_INC) -o "$@" -c "$<"

$(BUILDDIR)/%.o: %.cpp
	@echo "[CXX]\t$<"
	@mkdir -p "$(dir $@)"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(L_INC) -o "$@" -c "$<"

$(BUILDDIR)/%.o: %.ino
	@echo "[CXX]\t$<"
	@mkdir -p "$(dir $@)"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(L_INC) -o "$@" -x c++ -include Arduino.h -c "$<"

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@echo "[LD]\t$@"
	@$(CC) $(LDFLAGS) -o "$@" $(OBJS) $(LIBS)
	# @$(STRIP) "$@"

%.hex: %.elf
	@$(OBJCOPY) -S -x -O ihex -R .eeprom "$<" "$@"

# compiler generated dependency info
-include $(OBJS:.o=.d)

clean:
	@echo Cleaning...
	@rm -rf "$(BUILDDIR)"
	@rm -f "$(TARGET).elf" "$(TARGET).hex"
