# Makefile for NetShell Server and Client

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = 

# For MorphOS target, use different settings
ifdef MORPHOS
CC = ppc-morphos-gcc
CFLAGS += -noixemul
LDFLAGS += -noixemul
endif

# Target names
SERVER_TARGET = netshell
CLIENT_TARGET = netshell_client

# Source files
SERVER_SRC = netshell.c
CLIENT_SRC = netshell_client.c

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Server build
$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Client build
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# MorphOS build target
morphos: CFLAGS += -DMORPHOS
morphos: LDFLAGS += -DMORPHOS
morphos: $(SERVER_SRC) $(CLIENT_SRC)
	$(CC) $(CFLAGS) -DMORPHOS -o $(SERVER_TARGET) $(SERVER_SRC) $(LDFLAGS)
	$(CC) $(CFLAGS) -DMORPHOS -o $(CLIENT_TARGET) $(CLIENT_SRC) $(LDFLAGS)

# Install to MorphOS
install-morphos:
	scp $(SERVER_TARGET) $(CLIENT_TARGET) morphos@192.168.1.136:/Work/Development/git/netshell/

# Clean build artifacts
clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

# Test connection
test:
	@echo "Testing connection to MorphOS..."
	@echo ls | nc 192.168.1.136 2323

.PHONY: all clean install-morphos test morphos