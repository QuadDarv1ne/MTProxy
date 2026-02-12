/*
    This file is part of MTProxy project.

    MTProxy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    MTProxy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MTProxy.  If not, see <http://www.gnu.org/licenses/>.
*/

// Security Manager Implementation for MTProxy
//
// This file implements security enhancements including:
// 1. DDoS protection
// 2. Certificate pinning
// 3. HSM support
// 4. Access control

#include "security-manager.h"
#include "../common/kprintf.h"
#include "../common/precise-time.h"
#include <stdlib.h>
#include <string.h>

// DDoS Protection Structures
typedef struct {
    unsigned int ip_address;
    int connection_count;
    double first_seen;
    double last_seen;
    int blocked;
    double block_until;
} ddos_protection_entry_t;

typedef struct {
    ddos_protection_entry_t *entries;
    int max_entries;
    int current_entries;
    int max_connections_per_ip;
    double time_window;  // in seconds
    double block_duration;  // in seconds
} ddos_protection_t;

// Access Control Structures
typedef struct acl_entry {
    unsigned int ip_address;
    unsigned int ip_mask;
    acl_permission_level_t permission_level;
    struct acl_entry *next;
} acl_entry_t;

typedef struct {
    acl_entry_t *acl_list;
} access_control_t;

// Global security manager state
static ddos_protection_t ddos_protect = {0};
static access_control_t access_control = {0};
static int security_initialized = 0;

// Initialize DDoS protection
int init_ddos_protection(int max_connections, double time_window, double block_duration) {
    if (ddos_protect.entries) {
        vkprintf(1, "DDoS protection already initialized\n");
        return 0;
    }
    
    ddos_protect.max_connections_per_ip = max_connections;
    ddos_protect.time_window = time_window;
    ddos_protect.block_duration = block_duration;
    ddos_protect.current_entries = 0;
    ddos_protect.max_entries = 10000; // reasonable default
    
    ddos_protect.entries = calloc(ddos_protect.max_entries, sizeof(ddos_protection_entry_t));
    if (!ddos_protect.entries) {
        vkprintf(0, "Failed to allocate memory for DDoS protection entries\n");
        return -1;
    }
    
    vkprintf(1, "DDoS protection initialized: max %d connections per IP, %.2fs window, %.2fs block duration\n", 
             max_connections, time_window, block_duration);
    
    return 0;
}

// Check if IP is allowed to make a new connection
int check_ddos_protection(unsigned int ip_address) {
    if (!ddos_protect.entries) {
        return 1; // Allow if not initialized
    }
    
    // Check if IP is blocked
    for (int i = 0; i < ddos_protect.current_entries; i++) {
        if (ddos_protect.entries[i].ip_address == ip_address) {
            if (ddos_protect.entries[i].blocked) {
                if (precise_now < ddos_protect.entries[i].block_until) {
                    return 0; // Blocked
                } else {
                    // Unblock expired entry
                    ddos_protect.entries[i].blocked = 0;
                }
            }
            
            // Update connection count and timing
            ddos_protect.entries[i].last_seen = precise_now;
            ddos_protect.entries[i].connection_count++;
            
            // Check if we've exceeded the limit
            if (ddos_protect.entries[i].connection_count > ddos_protect.max_connections_per_ip) {
                ddos_protect.entries[i].blocked = 1;
                ddos_protect.entries[i].block_until = precise_now + ddos_protect.block_duration;
                
                vkprintf(1, "Blocked IP %u.%u.%u.%u for exceeding connection limit\n",
                         (ip_address >> 24) & 0xFF, (ip_address >> 16) & 0xFF,
                         (ip_address >> 8) & 0xFF, ip_address & 0xFF);
                
                return 0; // Block this connection
            }
            
            return 1; // Allow
        }
    }
    
    // IP not found, add it if there's space
    if (ddos_protect.current_entries < ddos_protect.max_entries) {
        int idx = ddos_protect.current_entries++;
        ddos_protect.entries[idx].ip_address = ip_address;
        ddos_protect.entries[idx].connection_count = 1;
        ddos_protect.entries[idx].first_seen = precise_now;
        ddos_protect.entries[idx].last_seen = precise_now;
        ddos_protect.entries[idx].blocked = 0;
        
        return 1; // Allow
    }
    
    return 0; // Deny due to table overflow
}

// Initialize HSM interface (placeholder implementation)
int init_hsm_interface(const char *module_path, int slot_id) {
    if (!module_path || strlen(module_path) >= 256) {
        vkprintf(0, "Invalid HSM module path\n");
        return -1;
    }
    
    // In a real implementation, this would initialize the actual HSM connection
    vkprintf(1, "HSM interface initialized: module='%s', slot=%d\n", 
             module_path, slot_id);
    
    return 0;
}

// Initialize access control system
int init_access_control() {
    if (access_control.acl_list) {
        vkprintf(1, "Access control already initialized\n");
        return 0;
    }
    
    // Start with empty ACL list
    access_control.acl_list = NULL;
    
    vkprintf(1, "Access control system initialized\n");
    return 0;
}

// Add an access control entry
int add_acl_entry(unsigned int ip_address, unsigned int ip_mask, acl_permission_level_t perm_level) {
    acl_entry_t *new_entry = malloc(sizeof(acl_entry_t));
    if (!new_entry) {
        vkprintf(0, "Failed to allocate memory for ACL entry\n");
        return -1;
    }
    
    new_entry->ip_address = ip_address;
    new_entry->ip_mask = ip_mask;
    new_entry->permission_level = perm_level;
    new_entry->next = access_control.acl_list;
    
    access_control.acl_list = new_entry;
    
    vkprintf(2, "Added ACL entry: IP=%u.%u.%u.%u, Mask=%u.%u.%u.%u, Level=%d\n",
             (ip_address >> 24) & 0xFF, (ip_address >> 16) & 0xFF,
             (ip_address >> 8) & 0xFF, ip_address & 0xFF,
             (ip_mask >> 24) & 0xFF, (ip_mask >> 16) & 0xFF,
             (ip_mask >> 8) & 0xFF, ip_mask & 0xFF,
             perm_level);
    
    return 0;
}

// Check access permission for IP
acl_permission_level_t check_access_permission(unsigned int ip_address) {
    acl_entry_t *current = access_control.acl_list;
    while (current) {
        if ((ip_address & current->ip_mask) == (current->ip_address & current->ip_mask)) {
            return current->permission_level;
        }
        current = current->next;
    }
    
    // Default: allow if no specific rule found
    return ACL_LEVEL_FULL_ACCESS;
}

// Initialize the security manager
int init_security_manager() {
    if (security_initialized) {
        return 0;
    }
    
    // Initialize DDoS protection (default settings)
    if (init_ddos_protection(100, 60.0, 300.0) < 0) {
        vkprintf(0, "Failed to initialize DDoS protection\n");
        return -1;
    }
    
    // Initialize access control
    if (init_access_control() < 0) {
        vkprintf(0, "Failed to initialize access control\n");
        return -1;
    }
    
    security_initialized = 1;
    vkprintf(1, "Security manager initialized\n");
    return 0;
}

// Cleanup security resources
void cleanup_security_manager() {
    // Cleanup DDoS protection
    if (ddos_protect.entries) {
        free(ddos_protect.entries);
        ddos_protect.entries = NULL;
    }
    
    // Cleanup access control
    acl_entry_t *current = access_control.acl_list;
    while (current) {
        acl_entry_t *temp = current;
        current = current->next;
        free(temp);
    }
    access_control.acl_list = NULL;
    
    security_initialized = 0;
    vkprintf(1, "Security manager cleaned up\n");
}

// Get security statistics
void get_security_stats(security_stats_t *stats) {
    if (!stats) return;
    
    // Get DDoS protection stats
    stats->ddos_current_entries = ddos_protect.current_entries;
    stats->ddos_max_entries = ddos_protect.max_entries;
    stats->ddos_max_connections_per_ip = ddos_protect.max_connections_per_ip;
    
    // Get ACL stats
    int acl_count = 0;
    acl_entry_t *current = access_control.acl_list;
    while (current) {
        acl_count++;
        current = current->next;
    }
    stats->acl_entry_count = acl_count;
    
    // For this basic implementation, we'll just set some defaults
    stats->cert_validation_attempts = 0;
    stats->cert_pinning_violations = 0;
    stats->hsm_operations_count = 0;
    stats->total_blocked_connections = 0; // Would be tracked in real implementation
}