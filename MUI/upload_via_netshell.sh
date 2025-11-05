#!/bin/sh
# Script to upload files to MorphOS via netshell connection
# This script uses base64 encoding to transfer binary-safe files through the netshell connection

MORPHOS_IP="192.168.1.136"
MORPHOS_PORT="2323"

# Change to the MUI directory
cd /common/active/sblo/Dev/netshell/MUI

echo "Creating directory on MorphOS..."
echo "mkdir -p /Work/Development/git/netshell/MUI" | nc $MORPHOS_IP $MORPHOS_PORT

# Function to upload a single file
upload_file() {
    local file_path=$1
    local remote_path=$2
    
    echo "Uploading $file_path to $remote_path..."
    
    # Encode the file in base64 and send it to MorphOS
    cat "$file_path" | base64 | ( \
        echo "base64 -d > $remote_path" && \
        cat \
    ) | nc $MORPHOS_IP $MORPHOS_PORT
}

# Upload each file
upload_file "gui_app.c" "/Work/Development/git/netshell/MUI/gui_app.c"
upload_file "gui_app.h" "/Work/Development/git/netshell/MUI/gui_app.h"
upload_file "Makefile" "/Work/Development/git/netshell/MUI/Makefile"
upload_file "README.md" "/Work/Development/git/netshell/MUI/README.md"

echo "All files uploaded. You can now connect to MorphOS and run 'make' in the /Work/Development/git/netshell/MUI directory."