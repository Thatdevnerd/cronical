#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

static int connection_count = 0;
static int max_connections = 10;

void* stats_thread(void* arg) {
    while (1) {
        printf("[%ds] -> Conns: [%d] Logins: [0] Ran: [0] -> Echoes: [0] Wgets: [0] TFTPs: [0]\n", 
               (int)time(NULL) % 100, connection_count);
        sleep(1);
    }
    return NULL;
}

int main() {
    printf("[DEBUG] Simple loader starting...\n");
    
    pthread_t stats_thrd;
    pthread_create(&stats_thrd, NULL, stats_thread, NULL);
    
    char line[1024];
    int line_count = 0;
    
    while (fgets(line, sizeof(line), stdin) != NULL && line_count < 20) {
        printf("[DEBUG] Processing line %d: %s", line_count + 1, line);
        
        // Simulate connection
        if (connection_count < max_connections) {
            connection_count++;
            printf("[DEBUG] Simulated connection %d\n", connection_count);
        }
        
        line_count++;
        usleep(50000); // 50ms delay
    }
    
    printf("[DEBUG] Simple loader completed successfully\n");
    return 0;
}
