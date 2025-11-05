#!/bin/sh
# Upload script for NetShell GUI to MorphOS

# Check if ssh is available
if ! command -v ssh &> /dev/null; then
    echo "SSH is required to upload files to MorphOS"
    exit 1
fi

# Check if the remote directory exists, create it if it doesn't
ssh morphos@192.168.1.136 "mkdir -p /Work/Development/git/netshell/MUI"

# Upload source files
scp gui_app.c gui_app.h Makefile README.md morphos@192.168.1.136:/Work/Development/git/netshell/MUI/

echo "Files uploaded. Connect to MorphOS and run 'make' in the /Work/Development/git/netshell/MUI directory."