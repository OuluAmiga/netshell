# Makefile for NetShell - Network Shell for MorphOS and Linux
# Supports cross-compilation for MorphOS using GCC in 'gg' directory

# Determine host architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Default values
CC_NATIVE := gcc
CFLAGS := -Wall -Wextra -O2 -std=gnu99
LDFLAGS := 

# MorphOS cross-compiler settings (for use when building ON MorphOS)
MORPHOS_GCC := ../gg/bin/ppc-morphos-gcc
MORPHOS_CFLAGS := -Wall -O2 -DMORPHOS -I../gg/include
MORPHOS_LDFLAGS := -L../gg/lib

# Target executable names
TARGET_NATIVE := netshell
TARGET_MORPHOS := netshell_mos

# Source files
SOURCES := netshell.c

.PHONY: all native morphos clean help

all: native

# Native build target
native: $(TARGET_NATIVE)

# MorphOS build target (only works when building ON MorphOS)
morphos: $(TARGET_MORPHOS)

# Build native executable
$(TARGET_NATIVE): $(SOURCES)
	$(CC_NATIVE) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

# Build MorphOS executable (only works when building ON MorphOS)
$(TARGET_MORPHOS): $(SOURCES)
	@if [ ! -f "$(MORPHOS_GCC)" ]; then \
		echo "Error: MorphOS GCC not found at $(MORPHOS_GCC)"; \
		echo "Make sure the 'gg' directory contains the MorphOS cross-compiler."; \
		exit 1; \
	fi
	$(MORPHOS_GCC) $(MORPHOS_CFLAGS) $(SOURCES) -o $@ $(MORPHOS_LDFLAGS)

clean:
	rm -f $(TARGET_NATIVE) $(TARGET_MORPHOS)

help:
	@echo "NetShell Makefile"
	@echo "Usage:"
	@echo "  make              - Build native version (default)"
	@echo "  make native       - Build native version"
	@echo "  make morphos      - Build for MorphOS using cross-compiler"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Cross-compilation requires the MorphOS GCC in the 'gg' directory."
	@echo "Expected path: $(MORPHOS_GCC)"