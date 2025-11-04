#!/bin/bash

# build.sh - Build script for NetShell
# Supports building both native and MorphOS versions

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "NetShell Build Script"
echo "====================="

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -n, --native     Build native version (default)"
    echo "  -m, --morphos    Build MorphOS version"
    echo "  -a, --all        Build all versions"
    echo "  -c, --clean      Clean build artifacts before building"
    echo "  -h, --help       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0               # Build native only"
    echo "  $0 -m            # Build MorphOS only"
    echo "  $0 -a            # Build all versions"
    echo "  $0 -c -a         # Clean and build all versions"
}

# Default options
BUILD_NATIVE=false
BUILD_MORPHOS=false
CLEAN_FIRST=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--native)
            BUILD_NATIVE=true
            shift
            ;;
        -m|--morphos)
            BUILD_MORPHOS=true
            shift
            ;;
        -a|--all)
            BUILD_NATIVE=true
            BUILD_MORPHOS=true
            shift
            ;;
        -c|--clean)
            CLEAN_FIRST=true
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

# Set defaults if no build target specified
if [ "$BUILD_NATIVE" = false ] && [ "$BUILD_MORPHOS" = false ]; then
    BUILD_NATIVE=true
fi

# Clean if requested
if [ "$CLEAN_FIRST" = true ]; then
    echo "Cleaning build artifacts..."
    make clean
    echo "Clean complete."
    echo ""
fi

# Build native version
if [ "$BUILD_NATIVE" = true ]; then
    echo "Building native version..."
    make native
    echo "Native build complete."
    echo ""
fi

# Build MorphOS version
if [ "$BUILD_MORPHOS" = true ]; then
    echo "Building MorphOS version..."
    
    # Check if MorphOS GCC is available
    if [ ! -f "../gg/bin/ppc-morphos-gcc" ]; then
        echo "Warning: MorphOS GCC not found at ../gg/bin/ppc-morphos-gcc"
        echo "Skipping MorphOS build."
        echo "Note: Cross-compilation requires setting up the MorphOS SDK in the 'gg' directory."
    else
        make morphos
        echo "MorphOS build complete."
    fi
    echo ""
fi

echo "Build process finished!"
echo ""
echo "Built executables:"
if [ "$BUILD_NATIVE" = true ]; then
    if [ -f "./netshell" ]; then
        echo "  - ./netshell (native)"
    fi
fi
if [ "$BUILD_MORPHOS" = true ]; then
    if [ -f "../gg/bin/ppc-morphos-gcc" ] && [ -f "./netshell_mos" ]; then
        echo "  - ./netshell_mos (MorphOS)"
    fi
fi