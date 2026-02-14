/*
 * Integration implementation for performance optimizer with MTProxy main engine
 */

#include "optimizer-integration.h"
#include "performance-optimizer.h"

// Initialize optimizer during MTProxy startup
int init_optimizer_integration(void) {
    // Initialize with aggressive optimization level for best performance
    return init_performance_optimizer(PERF_LEVEL_AGGRESSIVE);
}

// Update optimizer during runtime
int update_optimizer_integration(void) {
    return update_performance_optimizer();
}

// Cleanup optimizer during shutdown
void cleanup_optimizer_integration(void) {
    cleanup_performance_optimizer();
}

// Get current optimization status
perf_metrics_t get_optimizer_status(void) {
    return get_current_metrics();
}