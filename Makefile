# Make sure DEVKITPRO and DEVKITARM are set in your environment
ifeq ($(strip $(DEVKITPRO)),)
  $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

# Use devkitA64 toolchain for Switch (arm64)
CC := $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-gcc
CXX := $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-g++
STRIP := $(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-strip

# Use devkitPro libnx switch rules
include $(DEVKITPRO)/libnx/switch_rules

# Metadata (updated)
APP_TITLE := Title-Id-Lister
APP_AUTHOR := eradicatinglove
APP_VERSION := 1.0.0
# Remove NO_ICON since we want the icon to show

TARGET := Title-Id-Lister
BUILD := build
SOURCES := source
INCLUDES := include

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS := -g -Wall -O2 -ffunction-sections $(ARCH) -D__SWITCH__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(TARGET).map

LIBS := -lnx -ljpeg
LIBDIRS := $(PORTLIBS) $(LIBNX)

INCLUDE := -I$(INCLUDES) \
           $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
           -I$(BUILD)

LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# Files
CFILES := $(wildcard $(SOURCES)/*.c)
CPPFILES := $(wildcard $(SOURCES)/*.cpp)
SFILES := $(wildcard $(SOURCES)/*.s)

OFILES := $(patsubst $(SOURCES)/%.c,$(BUILD)/%.o,$(CFILES)) \
          $(patsubst $(SOURCES)/%.cpp,$(BUILD)/%.o,$(CPPFILES)) \
          $(patsubst $(SOURCES)/%.s,$(BUILD)/%.o,$(SFILES))

DEPENDS := $(OFILES:.o=.d)

# Linker
ifeq ($(strip $(CPPFILES)),)
  LD := $(CC)
else
  LD := $(CXX)
endif

# Build rules
.PHONY: all clean

all: $(TARGET).nro

# Embed icon.jpg as the icon shown in the homebrew menu
$(TARGET).nro: $(TARGET).elf $(TARGET).nacp icon.jpg
	@echo "[STRIP] $<"
	@$(STRIP) $<
	@echo "[NRO] $@"
	@elf2nro $< $@ --nacp=$(TARGET).nacp --icon=icon.jpg

$(TARGET).nacp:
	@echo "[NACP] $@"
	@nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@

$(TARGET).elf: $(OFILES)
	@echo "[LINK] $@"
	@$(LD) -o $@ $^ $(LIBPATHS) $(LIBS) $(LDFLAGS)

$(BUILD)/%.o: $(SOURCES)/%.c
	@mkdir -p $(BUILD)
	@echo "[CC] $<"
	@$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.cpp
	@mkdir -p $(BUILD)
	@echo "[CXX] $<"
	@$(CXX) -c $(CXXFLAGS) $(INCLUDE) $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.s
	@mkdir -p $(BUILD)
	@echo "[AS] $<"
	@$(CC) -c $(ASFLAGS) $< -o $@

-include $(DEPENDS)

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) *.elf *.nro *.map

