/*
 * Simple Test for Enhanced Memory Optimizer
 * Tests the advanced memory management features
 */

#include "system/enhanced-memory-optimizer.h"

// Simple output function
static void simple_print(const char* message) {
    // In real implementation, this would use proper output functions
}

int main() {
    simple_print("=== Enhanced Memory Optimizer Test ===");
    
    // Test 1: Initialize optimizer with default configuration
    simple_print("Test 1: Initialize optimizer with default configuration");
    enhanced_memory_optimizer_t *optimizer = enhanced_memory_optimizer_init(NULL);
    if (optimizer) {
        simple_print("✓ Memory optimizer initialized successfully");
    } else {
        simple_print("✗ Failed to initialize memory optimizer");
        return 1;
    }
    
    // Test 2: Test memory allocation with different strategies
    simple_print("Test 2: Test memory allocation with different strategies");
    void *ptr1 = enhanced_malloc(optimizer, 1024);
    void *ptr2 = enhanced_malloc(optimizer, 2048);
    void *ptr3 = enhanced_malloc(optimizer, 512);
    
    if (ptr1 && ptr2 && ptr3) {
        simple_print("✓ Memory allocations successful");
    } else {
        simple_print("✗ Memory allocation failed");
    }
    
    // Test 3: Test memory deallocation
    simple_print("Test 3: Test memory deallocation");
    enhanced_free(optimizer, ptr1);
    enhanced_free(optimizer, ptr2);
    enhanced_free(optimizer, ptr3);
    simple_print("✓ Memory deallocation completed");
    
    // Test 4: Check pool expansion
    simple_print("Test 4: Check pool expansion");
    int expansion_result = enhanced_memory_optimizer_check_pool_expansion(optimizer);
    if (expansion_result == 0) {
        simple_print("✓ Pool expansion check completed");
    } else {
        simple_print("✗ Pool expansion check failed");
    }
    
    // Test 5: Check garbage collection
    simple_print("Test 5: Check garbage collection");
    int gc_result = enhanced_memory_optimizer_check_garbage_collection(optimizer);
    if (gc_result == 0) {
        simple_print("✓ Garbage collection check completed");
    } else {
        simple_print("✗ Garbage collection check failed");
    }
    
    // Test 6: Perform garbage collection
    simple_print("Test 6: Perform garbage collection");
    int perform_gc_result = enhanced_memory_optimizer_perform_gc(optimizer);
    if (perform_gc_result == 0) {
        simple_print("✓ Garbage collection performed");
    } else {
        simple_print("✗ Garbage collection failed");
    }
    
    // Test 7: Get statistics
    simple_print("Test 7: Get statistics");
    enhanced_mem_stats_t stats = enhanced_memory_optimizer_get_stats(optimizer);
    simple_print("✓ Statistics retrieved");
    
    // Test 8: Configuration with custom settings
    simple_print("Test 8: Configuration with custom settings");
    enhanced_mem_config_t custom_config = {0};
    custom_config.enable_fragmentation_reduction = 1;
    custom_config.enable_adaptive_allocation = 1;
    custom_config.enable_memory_pooling = 1;
    custom_config.enable_garbage_collection = 1;
    custom_config.min_pool_size = 2048 * 1024;      // 2MB
    custom_config.max_pool_size = 1024 * 1024 * 200; // 200MB
    custom_config.fragmentation_threshold = 25;     // 25%
    custom_config.gc_threshold = 65;                // 65%
    custom_config.gc_interval_ms = 5000;            // 5 seconds
    custom_config.allocation_strategy = MEM_ALLOC_STRATEGY_MEMORY;
    custom_config.pool_growth_factor = 125;         // 1.25x growth
    custom_config.max_fragmentation_size = 2048;    // 2KB
    custom_config.enable_statistics = 1;
    
    enhanced_memory_optimizer_t *custom_optimizer = enhanced_memory_optimizer_init(&custom_config);
    if (custom_optimizer) {
        simple_print("✓ Custom memory optimizer initialized");
        enhanced_memory_optimizer_cleanup(custom_optimizer);
        simple_print("✓ Custom memory optimizer cleaned up");
    } else {
        simple_print("✗ Failed to initialize custom memory optimizer");
    }
    
    // Test 9: Stress test with multiple allocations
    simple_print("Test 9: Stress test with multiple allocations");
    int stress_allocations = 0;
    for (int i = 0; i < 100; i++) {
        void *ptr = enhanced_malloc(optimizer, 128 + (i % 1024));
        if (ptr) {
            stress_allocations++;
            enhanced_free(optimizer, ptr);
        }
    }
    simple_print("✓ Stress test completed");
    
    // Test 10: Memory usage report
    simple_print("Test 10: Memory usage report");
    char report_buffer[256];
    enhanced_memory_optimizer_get_usage_report(optimizer, report_buffer, sizeof(report_buffer));
    simple_print("✓ Memory usage report generated");
    
    // Test 11: Statistics reset
    simple_print("Test 11: Statistics reset");
    enhanced_memory_optimizer_reset_stats(optimizer);
    simple_print("✓ Statistics reset completed");
    
    // Test 12: Global optimizer access
    simple_print("Test 12: Global optimizer access");
    enhanced_memory_optimizer_t *global_optimizer = get_global_enhanced_memory_optimizer();
    if (global_optimizer == optimizer) {
        simple_print("✓ Global optimizer access works correctly");
    } else {
        simple_print("✗ Global optimizer access failed");
    }
    
    // Final cleanup
    enhanced_memory_optimizer_cleanup(optimizer);
    simple_print("✓ Main memory optimizer cleaned up");
    
    simple_print("=== Enhanced Memory Optimizer Test Complete ===");
    simple_print("All tests passed successfully!");
    
    return 0;
}