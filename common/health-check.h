/*
 * Health check system for MTProxy
 *
 * This file defines the interface for health check endpoints
 * to support container orchestration and monitoring systems.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Health status enumeration */
typedef enum {
    HEALTH_STATUS_UNKNOWN = 0,
    HEALTH_STATUS_HEALTHY,
    HEALTH_STATUS_UNHEALTHY,
    HEALTH_STATUS_DEGRADED
} health_status_t;

/* Health check result structure */
typedef struct health_result {
    char *component;              /* Component name */
    health_status_t status;       /* Health status */
    char *message;               /* Optional status message */
    uint64_t timestamp;          /* Timestamp of check */
    double response_time_ms;     /* Response time of check */
} health_result_t;

/* Health check function pointer type */
typedef health_result_t (*health_check_func)(void);

/* Health check provider */
typedef struct health_provider {
    char *name;                           /* Provider name */
    health_check_func check_function;     /* Function to run check */
    uint32_t interval_seconds;            /* Check interval */
    uint32_t timeout_seconds;             /* Check timeout */
    struct health_provider *next;         /* Next provider */
} health_provider_t;

/* Overall health status */
typedef struct {
    health_status_t overall_status;       /* Overall health status */
    uint64_t timestamp;                  /* Timestamp of status */
    health_result_t *individual_checks;   /* Individual check results */
    uint32_t check_count;                /* Number of individual checks */
    char *details;                       /* Additional details */
} overall_health_status_t;

/* Health endpoint types */
typedef enum {
    HEALTH_ENDPOINT_LIVENESS = 0,
    HEALTH_ENDPOINT_READINESS,
    HEALTH_ENDPOINT_STARTUP
} health_endpoint_type_t;

/* Initialize health check system */
int init_health_check_system(void);

/* Register a new health check provider */
int register_health_check_provider(const char *name, health_check_func func, uint32_t interval);

/* Run all registered health checks */
overall_health_status_t* run_health_checks(void);

/* Get liveness status */
health_status_t get_liveness_status(void);

/* Get readiness status */
health_status_t get_readiness_status(void);

/* Get startup status */
health_status_t get_startup_status(void);

/* Add custom health check */
int add_custom_health_check(const char *name, health_check_func func);

/* Remove health check provider */
int remove_health_check_provider(const char *name);

/* Get health status in JSON format */
char* get_health_status_json(health_endpoint_type_t type);

/* Get health status in plain text */
char* get_health_status_text(health_endpoint_type_t type);

/* Check if system is ready to serve traffic */
uint8_t is_system_ready(void);

/* Check if system is alive */
uint8_t is_system_alive(void);

/* Check if startup is complete */
uint8_t is_startup_complete(void);

/* Update health status based on internal metrics */
int update_health_status_from_metrics(void);

/* Set custom health status message */
int set_health_status_message(health_endpoint_type_t type, const char *message);

/* Get health check statistics */
void get_health_check_stats(uint32_t *total_checks, uint32_t *failed_checks);

/* Cleanup health check system */
void destroy_health_check_system(void);

/* Handle HTTP health check request */
int handle_health_check_request(health_endpoint_type_t type, char *response_buffer, size_t buffer_size);

/* Configure health check thresholds */
int configure_health_thresholds(double cpu_threshold, double mem_threshold, size_t disk_threshold);

#ifdef __cplusplus
}
#endif