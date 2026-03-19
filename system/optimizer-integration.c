/*
 * Integration implementation for performance optimizer with MTProxy main engine
 */

#include "optimizer-integration.h"
#include "performance-optimizer.h"

// Global optimizer context
static performance_optimizer_context_t g_optimizer_ctx;

// Initialize optimizer during MTProxy startup
int init_optimizer_integration(void) {
    // Initialize with advanced optimization level for best performance
    return perf_opt_init(&g_optimizer_ctx);
}

// Update optimizer during runtime
int update_optimizer_integration(void) {
    return perf_opt_run_optimization_cycle(&g_optimizer_ctx);
}

// Cleanup optimizer during shutdown
void cleanup_optimizer_integration(void) {
    perf_opt_cleanup(&g_optimizer_ctx);
}

// Get current optimization status
performance_optimizer_stats_t get_optimizer_status(void) {
    return perf_opt_get_stats(&g_optimizer_ctx);
}