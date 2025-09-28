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
