#!/bin/bash

# Bot Verification Script
# This script helps verify if bot binaries are running successfully

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Bot Verification Script ===${NC}"
echo

# Function to check if CNC server is running
check_cnc_server() {
    echo -e "${YELLOW}1. Checking CNC Server Status...${NC}"
    
    # Check if CNC is listening on port 666
    if netstat -tulpn 2>/dev/null | grep -q ":666 "; then
        echo -e "${GREEN}✓ CNC Server is running on port 666${NC}"
        netstat -tulpn | grep ":666 "
    else
        echo -e "${RED}✗ CNC Server is NOT running on port 666${NC}"
    fi
    
    # Alternative check with ss
    if ss -tulpn 2>/dev/null | grep -q ":666 "; then
        echo -e "${GREEN}✓ CNC Server confirmed with ss command${NC}"
        ss -tulpn | grep ":666 "
    fi
    echo
}

# Function to check for bot processes
check_bot_processes() {
    echo -e "${YELLOW}2. Checking for Bot Processes...${NC}"
    
    # Look for common bot process names
    local bot_names=("bot" "scan" "loader" "cnc" "stress" "ddos")
    local found_bots=0
    
    for name in "${bot_names[@]}"; do
        if pgrep -f "$name" >/dev/null 2>&1; then
            echo -e "${GREEN}✓ Found process containing '$name':${NC}"
            ps aux | grep "$name" | grep -v grep
            found_bots=1
        fi
    done
    
    if [ $found_bots -eq 0 ]; then
        echo -e "${RED}✗ No bot processes found${NC}"
    fi
    echo
}

# Function to check network connections
check_network_connections() {
    echo -e "${YELLOW}3. Checking Network Connections...${NC}"
    
    # Check for connections to CNC server
    local cnc_ip="136.175.200.15"
    local cnc_domain="bigbomboclaat.corestresser.cc"
    
    echo -e "${BLUE}Connections to CNC IP ($cnc_ip):${NC}"
    if netstat -an 2>/dev/null | grep -q "$cnc_ip"; then
        netstat -an | grep "$cnc_ip"
        echo -e "${GREEN}✓ Found connections to CNC IP${NC}"
    else
        echo -e "${RED}✗ No connections to CNC IP found${NC}"
    fi
    
    echo -e "${BLUE}Connections to CNC Domain ($cnc_domain):${NC}"
    if netstat -an 2>/dev/null | grep -q "$cnc_domain"; then
        netstat -an | grep "$cnc_domain"
        echo -e "${GREEN}✓ Found connections to CNC domain${NC}"
    else
        echo -e "${RED}✗ No connections to CNC domain found${NC}"
    fi
    echo
}

# Function to check system resources
check_system_resources() {
    echo -e "${YELLOW}4. Checking System Resources...${NC}"
    
    # Check CPU usage
    echo -e "${BLUE}Top CPU consuming processes:${NC}"
    ps aux --sort=-%cpu | head -10
    
    # Check memory usage
    echo -e "${BLUE}Memory usage:${NC}"
    free -h
    
    # Check disk usage
    echo -e "${BLUE}Disk usage:${NC}"
    df -h
    echo
}

# Function to check for suspicious network activity
check_suspicious_activity() {
    echo -e "${YELLOW}5. Checking for Suspicious Network Activity...${NC}"
    
    # Check for high connection counts
    echo -e "${BLUE}Active network connections:${NC}"
    netstat -an | wc -l
    echo "Total connections: $(netstat -an | wc -l)"
    
    # Check for SYN flood patterns
    echo -e "${BLUE}SYN connections:${NC}"
    netstat -an | grep SYN | wc -l
    echo "SYN connections: $(netstat -an | grep SYN | wc -l)"
    
    # Check for UDP connections
    echo -e "${BLUE}UDP connections:${NC}"
    netstat -an | grep UDP | wc -l
    echo "UDP connections: $(netstat -an | grep UDP | wc -l)"
    echo
}

# Function to check log files
check_logs() {
    echo -e "${YELLOW}6. Checking System Logs...${NC}"
    
    # Check for recent network activity in logs
    echo -e "${BLUE}Recent network activity in system logs:${NC}"
    if [ -f /var/log/messages ]; then
        tail -20 /var/log/messages | grep -i "network\|connection\|socket" || echo "No recent network activity in messages"
    fi
    
    if [ -f /var/log/syslog ]; then
        tail -20 /var/log/syslog | grep -i "network\|connection\|socket" || echo "No recent network activity in syslog"
    fi
    echo
}

# Function to test bot binary directly
test_bot_binary() {
    echo -e "${YELLOW}7. Testing Bot Binary...${NC}"
    
    local binary_dir="/root/binaries"
    
    if [ -d "$binary_dir" ]; then
        echo -e "${BLUE}Available bot binaries:${NC}"
        ls -la "$binary_dir"/* 2>/dev/null || echo "No binaries found"
        
        # Test if binaries are executable
        for binary in "$binary_dir"/*; do
            if [ -f "$binary" ]; then
                echo -e "${BLUE}Testing binary: $(basename "$binary")${NC}"
                echo "File type: $(file "$binary")"
                echo "Size: $(stat -c%s "$binary") bytes"
                echo "Permissions: $(ls -la "$binary")"
                
                # Check if binary is statically linked
                if ldd "$binary" 2>/dev/null | grep -q "not a dynamic executable"; then
                    echo -e "${GREEN}✓ Binary is statically linked${NC}"
                else
                    echo -e "${YELLOW}! Binary appears to be dynamically linked${NC}"
                fi
                echo
            fi
        done
    else
        echo -e "${RED}✗ Binary directory not found: $binary_dir${NC}"
    fi
    echo
}

# Function to check CNC database
check_cnc_database() {
    echo -e "${YELLOW}8. Checking CNC Database...${NC}"
    
    # Check if MariaDB is running
    if systemctl is-active --quiet mariadb || systemctl is-active --quiet mysql; then
        echo -e "${GREEN}✓ Database service is running${NC}"
        
        # Try to connect to database
        if mysql -u root -pRunescapex@1 -e "SHOW DATABASES;" 2>/dev/null; then
            echo -e "${GREEN}✓ Database connection successful${NC}"
            
            # Check for bot-related tables
            echo -e "${BLUE}Checking for bot-related tables:${NC}"
            mysql -u root -pRunescapex@1 -e "USE AJAX; SHOW TABLES;" 2>/dev/null || echo "Could not access AJAX database"
        else
            echo -e "${RED}✗ Database connection failed${NC}"
        fi
    else
        echo -e "${RED}✗ Database service is not running${NC}"
    fi
    echo
}

# Main execution
echo -e "${BLUE}Starting comprehensive bot verification...${NC}"
echo

check_cnc_server
check_bot_processes
check_network_connections
check_system_resources
check_suspicious_activity
check_logs
test_bot_binary
check_cnc_database

echo -e "${BLUE}=== Verification Complete ===${NC}"
echo
echo -e "${YELLOW}Summary of verification methods:${NC}"
echo "1. Check if CNC server is running on port 666"
echo "2. Look for bot processes in process list"
echo "3. Monitor network connections to CNC server"
echo "4. Check system resource usage"
echo "5. Look for suspicious network activity"
echo "6. Check system logs for network activity"
echo "7. Test bot binary properties"
echo "8. Verify CNC database connectivity"
echo
echo -e "${GREEN}For real-time monitoring, use:${NC}"
echo "• watch -n 1 'netstat -tulpn | grep :666'"
echo "• watch -n 1 'ps aux | grep bot'"
echo "• tail -f /var/log/messages"
