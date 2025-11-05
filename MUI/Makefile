# Makefile for NetShell MUI GUI Application

# Compiler and tools
CC = gcc
STRIP = strip

# On MorphOS, the SDK is typically in the standard location
MORPHOSSDK = SDK:

# Compiler flags for MorphOS
CFLAGS = -noixemul -I$(MORPHOSSDK)Local/Include -I$(MORPHOSSDK)Local/Include/inline -I$(MORPHOSSDK)Local/Include/mui -D__MORPHOS__
LDFLAGS = -L$(MORPHOSSDK)Local/Libs -L$(MORPHOSSDK)Local/Libs/mui -lamiga -lmui

# Target
TARGET = netshell_gui
SOURCES = netshell_gui.c

# Build the application
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)
	$(STRIP) $(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Upload to MorphOS via netshell connection
upload:
	@echo "Uploading files to MorphOS..."
	ssh morphos@192.168.1.136 "mkdir -p /Work/Development/git/netshell/MUI" && \
	scp $(SOURCES) Makefile morphos@192.168.1.136:/Work/Development/git/netshell/MUI/
	@echo "Upload complete. Connect to MorphOS and run 'make' in the /Work/Development/git/netshell/MUI directory."

# Test connection to MorphOS netshell server
test:
	@echo "Testing connection to MorphOS netshell server..."
	@echo ls | nc 192.168.1.136 2323

.PHONY: clean upload test