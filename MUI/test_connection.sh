#!/bin/sh
# Connection test script for MorphOS

echo "Testing connection to MorphOS system..."
echo "Running 'ls' command on MorphOS via netshell server..."

result=$(echo ls | nc 192.168.1.136 2323)
if [ $? -eq 0 ]; then
    echo "Connection successful!"
    echo "Remote directory listing:"
    echo "$result"
    echo ""
    echo "System appears to be accessible."
else
    echo "Connection failed!"
    exit 1
fi