# NetShell

NetShell is a simple network shell server that allows remote execution of shell commands over TCP/IP. It's designed to work on both MorphOS and Linux systems.

## Features

- Simple TCP server that accepts shell connections
- For each client connection, forks a new shell process
- Supports both MorphOS and Linux platforms
- MorphOS-specific optimizations using ixemul layer

## Building

### On Linux (Cross-compilation for MorphOS)

```bash
make morphos
```

### On MorphOS

```bash
make
```

This will build the MorphOS version by default since the Makefile detects MorphOS environment.

## Usage

The server listens on the default port (2324) unless specified otherwise:

```bash
./netshell [port]
```

Connect to the server using any TCP client (like telnet or netcat):

```bash
telnet <server_ip> <port>
```

## Architecture Notes

- On MorphOS: Uses `vfork()` instead of `fork()` due to limitations in the ixemul layer
- On Linux: Uses standard `fork()` for process creation
- Process synchronization handled with `waitpid()`
- Socket connection handling with proper cleanup

## Known Issues

- Argument parsing has been removed in MorphOS build for compatibility
- Uses `_exit()` instead of `exit()` in child processes after `vfork()` to comply with POSIX requirements

## File Structure

- `netshell.c`: Main source code
- `Makefile`: Build configuration
- `README.md`: This file
- `MUI/`: GUI application directory

## License

This project is in the public domain.

## GUI Version

A GUI version of NetShell exists in the gui_version/ directory using Intuition (Classic Amiga GUI) rather than MUI.
The GUI version features:
- Start/Stop buttons for server control
- Port configuration
- Status display
- Log output area
- Settings persistence

While MUI development is possible as shown in the MUI/ directory, the existing Intuition-based implementation is functional and recommended.