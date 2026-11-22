# Nintendo 3DS homebrew build (devkitARM + libctru).
# Run from the repository root: make -f 3ds/Makefile
.SUFFIXES:

TARGET := XAVRX-WII
BUILD := build-3ds
SOURCES := src/core src/cpu src/memory src/system src/platform
INCLUDES := include

APP_TITLE := XAVRX-WII
APP_DESCRIPTION := Experimental Wii emulator foundation
APP_AUTHOR := XAVRX-WII

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -O2 -Wall -Wextra -mword-relocations -ffunction-sections -fdata-sections $(ARCH) -D__3DS__
CXXFLAGS := $(CFLAGS) -std=gnu++20
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=3dsx.specs $(ARCH) -Wl,--gc-sections
LIBS := -lctru -lm

include $(DEVKITPRO)/devkitARM/3ds_rules
