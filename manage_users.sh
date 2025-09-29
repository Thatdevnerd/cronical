#!/bin/bash

# User Management Script for CNC Database
# This script helps manage users in the AJAX database

DB_HOST="127.0.0.1"
DB_USER="root"
DB_PASS="Runescapex@1"
DB_NAME="AJAX"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== CNC User Management Script ===${NC}"

# Function to list all users
list_users() {
    echo -e "${YELLOW}Current users in database:${NC}"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e "SELECT username, admin, max_bots, cooldown, duration_limit FROM users;"
}

# Function to add admin user
add_admin() {
    local username="$1"
    local password="$2"
    
    if [ -z "$username" ] || [ -z "$password" ]; then
        echo -e "${RED}Usage: add_admin <username> <password>${NC}"
        return 1
    fi
    
    echo -e "${YELLOW}Adding admin user: $username${NC}"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e "INSERT INTO users (username, password, max_bots, admin, last_paid, cooldown, duration_limit) VALUES ('$username', '$password', 0, 1, UNIX_TIMESTAMP(), 0, 0);"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Admin user '$username' added successfully${NC}"
    else
        echo -e "${RED}Failed to add admin user${NC}"
    fi
}

# Function to add regular user
add_user() {
    local username="$1"
    local password="$2"
    local max_bots="${3:-100}"
    local cooldown="${4:-30}"
    local duration_limit="${5:-300}"
    
    if [ -z "$username" ] || [ -z "$password" ]; then
        echo -e "${RED}Usage: add_user <username> <password> [max_bots] [cooldown] [duration_limit]${NC}"
        return 1
    fi
    
    echo -e "${YELLOW}Adding regular user: $username${NC}"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e "INSERT INTO users (username, password, max_bots, admin, last_paid, cooldown, duration_limit) VALUES ('$username', '$password', $max_bots, 0, UNIX_TIMESTAMP(), $cooldown, $duration_limit);"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}User '$username' added successfully${NC}"
    else
        echo -e "${RED}Failed to add user${NC}"
    fi
}

# Function to remove user
remove_user() {
    local username="$1"
    
    if [ -z "$username" ]; then
        echo -e "${RED}Usage: remove_user <username>${NC}"
        return 1
    fi
    
    echo -e "${YELLOW}Removing user: $username${NC}"
    mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" -e "DELETE FROM users WHERE username = '$username';"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}User '$username' removed successfully${NC}"
    else
        echo -e "${RED}Failed to remove user${NC}"
    fi
}

# Main script logic
case "$1" in
    "list")
        list_users
        ;;
    "addadmin")
        add_admin "$2" "$3"
        ;;
    "adduser")
        add_user "$2" "$3" "$4" "$5" "$6"
        ;;
    "remove")
        remove_user "$2"
        ;;
    *)
        echo -e "${YELLOW}Usage: $0 {list|addadmin|adduser|remove}${NC}"
        echo -e "${YELLOW}Examples:${NC}"
        echo -e "  $0 list"
        echo -e "  $0 addadmin username password"
        echo -e "  $0 adduser username password [max_bots] [cooldown] [duration_limit]"
        echo -e "  $0 remove username"
        ;;
esac
