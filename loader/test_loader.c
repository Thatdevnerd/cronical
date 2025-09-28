#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    printf("[DEBUG] Simple loader test starting...\n");
    
    char line[1024];
    int line_count = 0;
    
    while (fgets(line, sizeof(line), stdin) != NULL && line_count < 5) {
        printf("[DEBUG] Processing line %d: %s", line_count + 1, line);
        line_count++;
        usleep(100000); // 100ms delay
    }
    
    printf("[DEBUG] Test completed successfully\n");
    return 0;
}
