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

#pragma once

#include <stdint.h>

// Permission levels for access control
typedef enum {
    ACL_LEVEL_NONE = 0,        // No access
    ACL_LEVEL_LIMITED = 1,     // Limited access
    ACL_LEVEL_STANDARD = 2,    // Standard access
    ACL_LEVEL_FULL_ACCESS = 3  // Full access
} acl_permission_level_t;

// Security statistics structure
typedef struct {
    uint32_t ddos_current_entries;
    uint32_t ddos_max_entries;
    uint32_t ddos_max_connections_per_ip;
    uint32_t acl_entry_count;
    uint32_t cert_validation_attempts;
    uint32_t cert_pinning_violations;
    uint32_t hsm_operations_count;
    uint32_t total_blocked_connections;
} security_stats_t;

// Function declarations for security manager
int init_security_manager(void);
void cleanup_security_manager(void);
int check_ddos_protection(unsigned int ip_address);
int init_hsm_interface(const char *module_path, int slot_id);
int init_access_control(void);
int add_acl_entry(unsigned int ip_address, unsigned int ip_mask, acl_permission_level_t perm_level);
acl_permission_level_t check_access_permission(unsigned int ip_address);
void get_security_stats(security_stats_t *stats);