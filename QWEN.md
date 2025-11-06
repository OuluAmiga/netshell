# NetShell Project Documentation

## Overview
This project is hosted on the remote MorphOS computer at /Work/Development/git/netshell.

The MorphOS SDK is located locally at /home/sblo/xtra/morphos/Development/ but its executables cannot be run locally.

## Protocol and Features

### Enhanced Protocol
The NetShell system uses an enhanced protocol with magic string negotiation:

- **Magic String**: "NETSHELL_EXTENDED_V1" sent as first message to activate binary extensions
- **Server Response**: "EXTENDED_ACK" confirms extended protocol support
- **Fallback**: Basic text mode when extensions are not supported
- **Backward Compatibility**: Full netcat compatibility maintained

### Binary Encoding Support
- **Both client and server** support binary encoding for messages
- **File transfer capabilities** with SEND_FILE and GET_FILE commands
- **Ncurses application support** with proper binary data handling
- **Safe binary data transfer** without corruption

## Usage

### Basic Usage
```bash
# Traditional usage (maintained for compatibility)
echo "ls -la" | nc 192.168.1.136 2324

# Direct server execution
./netshell [port]  # Default port is 2324
```

### Enhanced Client Usage
```bash
# Execute command directly (eval mode)
./netshell_client -e "ls -la" 192.168.1.136 [port]
./netshell_client --eval "command" hostname [port]

# Execute command from file (new feature)
./netshell_client -E script.sh 192.168.1.136 [port]
./netshell_client --eval-file script.sh hostname [port]

# Session-based connections
./netshell_client -S myserver 192.168.1.136 [port]  # Save session
./netshell_client -S myserver -a 192.168.1.136 -p 2324 -u username -desc "Description"  # Save with config
./netshell_client myserver                          # Use saved session
./netshell_client -l                                # List sessions
./netshell_client -s myserver                       # Explicitly use session

# Default session management
./netshell_client -d myserver                       # Set myserver as default
./netshell_client -D                                # Unset default session
./netshell_client -e "ls"                           # Use default session (if set)

# Interactive mode
./netshell_client 192.168.1.136 [port]

# File transfer (in interactive mode)
send_file local_file remote_file
get_file remote_file local_file

# Ncurses applications
ncurses vim
ncurses less filename
```

### MUI Applications
```bash
# Server management GUI
./netshell_server_gui

# Client GUI
./netshell_client_gui
```

## Build System

### Cross-platform compilation
```bash
make                    # Build server and client
make morphos            # Build for MorphOS with -noixemul flag
make all                # Build all components including MUI apps
```

### MorphOS targeting
- Uses `-noixemul` flag for native MorphOS applications
- PowerPC architecture compilation
- Not Amiga emulation

## Architecture Components

### Server (netshell)
- Enhanced protocol support with magic string negotiation
- Binary-safe file transfer
- Ncurses compatibility
- Maintains netcat compatibility

### Client (netshell_client)
- -e/--eval function for direct command execution
- Session management with ~/.config/netshell/
- File transfer capabilities
- Interactive mode
- Ncurses application support

### MUI Applications
- Server GUI (netshell_server_gui) - for server management
- Client GUI (netshell_client_gui) - for client operations

## Configuration
- Sessions saved to ~/.config/netshell/
- Automatic directory creation
- Session metadata tracking (last used time)

## Development Notes

### MorphOS MUI Development
- Use basic MUI macros like ApplicationObject, WindowObject, VGroup, HGroup
- Use SimpleButton() for basic buttons
- Use StringObject for string input
- Use TextObject for text display
- Use ScrollgroupObject for scrollable content
- Include basic MUI headers like <libraries/mui.h>
- Don't use advanced MUI object constants that might not be available

### MorphOS Build Flags
- When building on MorphOS, the compiler flags are different than in cross-compilation
- Instead of "-mc68020 -m68881", MorphOS GCC uses "-noixemul" for native applications
- MorphOS SDK path is typically "SDK:" rather than Unix-style paths
- MorphOS GCC doesn't recognize the "-mc68020" and "-m68881" flags