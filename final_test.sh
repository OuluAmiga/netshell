#!/bin/bash

# final_test.sh - Final test script for netshell

echo "Final test of netshell server..."

# Compile the netshell
cd /home/sblo/xtra/morphos/Development/netshell
make clean && make

# Kill any existing instances
pkill netshell 2>/dev/null
sleep 2

# Start the server in the background
timeout 20s ./netshell 2323 > /tmp/netshell_final_test.log 2>&1 &
SERVER_PID=$!

echo "Started netshell server with PID $SERVER_PID"

# Give it time to start
sleep 3

# Check if it's running and listening
if netstat -tuln | grep -q ":2323 "; then
    echo "Server is listening on port 2323"
    
    # Test with a few commands
    echo "Testing pwd command..."
    echo -e "pwd\nexit" | timeout 5s nc localhost 2323 > /tmp/pwd_output.log 2>&1
    
    sleep 2
    
    echo "Testing ls command..."
    echo -e "ls -l\nexit" | timeout 5s nc localhost 2323 > /tmp/ls_output.log 2>&1
    
    sleep 2
    
    echo "Testing whoami command..."
    echo -e "whoami\nexit" | timeout 5s nc localhost 2323 > /tmp/whoami_output.log 2>&1
    
    # Wait a moment for all tests to complete
    sleep 3
    
    # Kill server if still running
    if ps -p $SERVER_PID > /dev/null; then
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
    fi
    
    echo "Server stopped"
    
    # Show results
    echo "=== Server Log ==="
    cat /tmp/netshell_final_test.log
    
    echo "=== pwd Output ==="
    cat /tmp/pwd_output.log
    
    echo "=== ls Output ==="
    cat /tmp/ls_output.log
    
    echo "=== whoami Output ==="
    cat /tmp/whoami_output.log
    
    # Cleanup
    rm -f /tmp/netshell_final_test.log /tmp/pwd_output.log /tmp/ls_output.log /tmp/whoami_output.log
else
    echo "Server is not listening on port 2323"
    
    # Try to kill server
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    
    echo "=== Server Log ==="
    cat /tmp/netshell_final_test.log
    
    rm -f /tmp/netshell_final_test.log
fi