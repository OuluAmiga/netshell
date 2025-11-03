#!/bin/bash

# run.sh - Run script for NetShell
# Provides easy ways to start the server with various options

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "NetShell Run Script"
echo "==================="

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -p, --port PORT      Specify port to listen on (default: 2323)"
    echo "  -b, --build-first    Build before running"
    echo "  -h, --help           Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                   # Run on default port (2323)"
    echo "  $0 -p 4567           # Run on custom port"
    echo "  $0 -b -p 8080        # Build and run on port 8080"
}

# Default values
PORT=2323
BUILD_FIRST=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--port)
            PORT="$2"
            if [[ ! "$PORT" =~ ^[0-9]+$ ]] || [ "$PORT" -lt 1 ] || [ "$PORT" -gt 65535 ]; then
                echo "Error: Invalid port number: $PORT"
                exit 1
            fi
            shift 2
            ;;
        -b|--build-first)
            BUILD_FIRST=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Build if requested
if [ "$BUILD_FIRST" = true ]; then
    echo "Building NetShell..."
    make clean
    make
    echo ""
fi

# Check if executable exists
if [ ! -f "./netshell" ]; then
    echo "Error: Executable './netshell' not found."
    echo "Please build it first with: make"
    exit 1
fi

# Show run information
echo "Starting NetShell server on port $PORT..."
echo "Connect with: telnet localhost $PORT"
echo "Press Ctrl+C to stop the server"
echo ""

# Start the server
./netshell "$PORT"