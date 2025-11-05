# NetShell Protocol Specification

## Overview
The NetShell protocol allows remote command execution with optional binary extensions.

## Base Protocol
- Connection established on configured TCP port
- Server spawns shell for each client
- Text commands sent from client to server
- Server output sent back to client

## Extended Protocol Mode
Extended protocol is activated when client sends the magic string "NETSHELL_EXTENDED_V1" followed by a newline character as the very first message after connecting.

### Magic String
- Client sends: "NETSHELL_EXTENDED_V1\n" (19 bytes)
- This must be the first message after connection
- Server responds with "EXTENDED_ACK\n" (13 bytes) if supported

### Extended Protocol Commands
The extended protocol supports the following binary-safe commands:

#### File Transfer Commands
- `SEND_FILE <filename> <size>` - Client wants to send a file
  - Server responds with "READY" to accept or "DENY" to reject
  - Client then sends raw file bytes
  - Server responds with "OK" or "ERROR" after receiving

- `GET_FILE <filename>` - Client wants to retrieve a file
  - Server responds with "SIZE <file_size>" and file bytes if exists
  - Server responds with "NOT_FOUND" if file doesn't exist

#### Binary Data Commands
- `BINARY_START` - Begin binary data mode
- `BINARY_END` - End binary data mode

#### Standard Commands (backward compatible)
- Text commands are passed to shell as usual

### Fallback Mode
If client doesn't send the magic string or server doesn't support extensions, the server operates in basic text mode (original behavior).