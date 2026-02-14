/*
 * Integration header for performance optimizer with MTProxy main engine
 */

#ifndef _OPTIMIZER_INTEGRATION_H_
#define _OPTIMIZER_INTEGRATION_H_

#include "performance-optimizer.h"

// Function to initialize optimizer during MTProxy startup
int init_optimizer_integration(void);

// Function to update optimizer during runtime
int update_optimizer_integration(void);

// Function to cleanup optimizer during shutdown
void cleanup_optimizer_integration(void);

// Function to get current optimization status
perf_metrics_t get_optimizer_status(void);

#endif