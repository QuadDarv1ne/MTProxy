/*
 * test_grpc_server.c — gRPC server tests for MTProxy
 *
 * Tests for gRPC server initialization, configuration, and basic operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "api/grpc-server.h"
#include "testing/test_common.h"

/* ============================================================================
 * Helper functions
 * ============================================================================ */

static void print_server_status(const grpc_server_status_t *status) {
    printf("    Server status:\n");
    printf("      State: %s\n", grpc_server_state_to_string(status->state));
    printf("      Version: %s\n", status->version);
    printf("      Commit: %s\n", status->commit_hash);
    printf("      Uptime: %ld seconds\n", (long)status->uptime_seconds);
    printf("      Platform: %s %s\n", status->platform.os, status->platform.arch);
}

static void print_server_stats(const grpc_server_stats_t *stats) {
    printf("    Server stats:\n");
    printf("      Total calls: %ld\n", (long)stats->total_calls);
    printf("      Total errors: %ld\n", (long)stats->total_errors);
    printf("      Active streams: %ld\n", (long)stats->active_streams);
}

/* ============================================================================
 * Test cases
 * ============================================================================ */

/**
 * Test 1: gRPC server initialization
 */
TEST(test_grpc_server_init) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));

    config.enabled = true;
    config.port = 50051;
    strncpy(config.bind_address, "127.0.0.1", sizeof(config.bind_address) - 1);
    config.max_connections = 100;
    config.enable_reflection = true;

    int result = grpc_server_init(&ctx, &config);

    ASSERT_EQ(0, result, "grpc_server_init should return 0");
    ASSERT_TRUE(ctx.initialized, "Context should be initialized");
    ASSERT_FALSE(ctx.running, "Context should not be running");
    ASSERT_EQ(config.port, ctx.config.port, "Port should match");

    return 0;
}

/**
 * Test 2: gRPC server start/stop
 */
TEST(test_grpc_server_start_stop) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));

    config.enabled = true;
    config.port = 50052;
    config.max_connections = 50;

    int result = grpc_server_init(&ctx, &config);
    ASSERT_EQ(0, result, "Initialization should succeed");

    result = grpc_server_start(&ctx);
    ASSERT_EQ(0, result, "Start should succeed");
    ASSERT_TRUE(ctx.running, "Server should be running");
    ASSERT_EQ(GRPC_SERVER_STATE_RUNNING, ctx.status.state, "State should be RUNNING");

    // Small delay to let server start
    struct timespec ts = {0, 100000000}; // 100ms
    nanosleep(&ts, NULL);

    grpc_server_stop(&ctx);
    ASSERT_FALSE(ctx.running, "Server should not be running after stop");
    ASSERT_EQ(GRPC_SERVER_STATE_STOPPED, ctx.status.state, "State should be STOPPED");

    grpc_server_cleanup(&ctx);
    ASSERT_FALSE(ctx.initialized, "Context should be cleaned up");

    return 0;
}

/**
 * Test 3: gRPC server status callback
 */
TEST(test_grpc_server_status_callback) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;
    grpc_server_status_t status;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));
    memset(&status, 0, sizeof(status));

    config.enabled = true;
    config.port = 50053;

    int result = grpc_server_init(&ctx, &config);
    ASSERT_EQ(0, result, "Initialization should succeed");

    // Test default callback
    result = ctx.get_status_callback(&status);
    ASSERT_EQ(0, result, "get_status_callback should succeed");
    ASSERT_EQ(GRPC_SERVER_STATE_RUNNING, status.state, "Default state should be RUNNING");
    ASSERT_TRUE(strlen(status.version) > 0, "Version should not be empty");

    print_server_status(&status);

    return 0;
}

/**
 * Test 4: gRPC server custom callback
 */
static int custom_get_status_callback(grpc_server_status_t *status) {
    if (!status) return -1;

    memset(status, 0, sizeof(grpc_server_status_t));
    status->state = GRPC_SERVER_STATE_RUNNING;
    strncpy(status->version, "test-1.0.0", sizeof(status->version) - 1);
    strncpy(status->commit_hash, "test-commit", sizeof(status->commit_hash) - 1);
    status->uptime_seconds = 3600;
    status->start_time_unix = time(NULL) - 3600;
    strncpy(status->platform.os, "test-os", sizeof(status->platform.os) - 1);
    strncpy(status->platform.arch, "test-arch", sizeof(status->platform.arch) - 1);
    status->platform.cpu_cores = 4;
    status->platform.total_memory_bytes = 8LL * 1024 * 1024 * 1024;

    return 0;
}

TEST(test_grpc_server_custom_callback) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;
    grpc_server_status_t status;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));
    memset(&status, 0, sizeof(status));

    config.enabled = true;
    config.port = 50054;

    int result = grpc_server_init(&ctx, &config);
    ASSERT_EQ(0, result, "Initialization should succeed");

    // Set custom callback
    grpc_server_set_status_callback(&ctx, custom_get_status_callback);

    result = ctx.get_status_callback(&status);
    ASSERT_EQ(0, result, "Custom callback should succeed");
    ASSERT_EQ(GRPC_SERVER_STATE_RUNNING, status.state, "State should match");
    ASSERT_STR_EQ("test-1.0.0", status.version, "Version should match custom callback");
    ASSERT_STR_EQ("test-commit", status.commit_hash, "Commit should match custom callback");
    ASSERT_EQ(3600, status.uptime_seconds, "Uptime should match custom callback");

    return 0;
}

/**
 * Test 5: gRPC server statistics
 */
TEST(test_grpc_server_statistics) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;
    grpc_server_stats_t stats;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));
    memset(&stats, 0, sizeof(stats));

    config.enabled = true;
    config.port = 50055;

    int result = grpc_server_init(&ctx, &config);
    ASSERT_EQ(0, result, "Initialization should succeed");

    // Get stats (should be zero initially)
    grpc_server_get_stats(&ctx, &stats);
    ASSERT_EQ(0, stats.total_calls, "Total calls should be 0");
    ASSERT_EQ(0, stats.total_errors, "Total errors should be 0");

    // Reset stats
    grpc_server_reset_stats(&ctx);
    grpc_server_get_stats(&ctx, &stats);
    ASSERT_EQ(0, stats.total_calls, "Total calls should be 0 after reset");

    print_server_stats(&stats);

    return 0;
}

/**
 * Test 6: gRPC server state conversion
 */
TEST(test_grpc_server_state_conversion) {
    const char *state_str;
    grpc_server_state_t state;

    // Test all states
    state_str = grpc_server_state_to_string(GRPC_SERVER_STATE_UNKNOWN);
    ASSERT_STR_EQ("unknown", state_str, "UNKNOWN state should match");

    state_str = grpc_server_state_to_string(GRPC_SERVER_STATE_STARTING);
    ASSERT_STR_EQ("starting", state_str, "STARTING state should match");

    state_str = grpc_server_state_to_string(GRPC_SERVER_STATE_RUNNING);
    ASSERT_STR_EQ("running", state_str, "RUNNING state should match");

    state_str = grpc_server_state_to_string(GRPC_SERVER_STATE_STOPPING);
    ASSERT_STR_EQ("stopping", state_str, "STOPPING state should match");

    state_str = grpc_server_state_to_string(GRPC_SERVER_STATE_STOPPED);
    ASSERT_STR_EQ("stopped", state_str, "STOPPED state should match");

    state_str = grpc_server_state_to_string(GRPC_SERVER_STATE_ERROR);
    ASSERT_STR_EQ("error", state_str, "ERROR state should match");

    // Test reverse conversion
    state = grpc_server_state_from_string("running");
    ASSERT_EQ(GRPC_SERVER_STATE_RUNNING, state, "String to state conversion should match");

    state = grpc_server_state_from_string("unknown");
    ASSERT_EQ(GRPC_SERVER_STATE_UNKNOWN, state, "Unknown string should map to UNKNOWN");

    state = grpc_server_state_from_string(NULL);
    ASSERT_EQ(GRPC_SERVER_STATE_UNKNOWN, state, "NULL should map to UNKNOWN");

    return 0;
}

/**
 * Test 7: gRPC server is_running check
 */
TEST(test_grpc_server_is_running) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));

    config.enabled = true;
    config.port = 50056;

    // Not initialized
    ASSERT_FALSE(grpc_server_is_running(&ctx), "Not initialized should not be running");

    int result = grpc_server_init(&ctx, &config);
    ASSERT_EQ(0, result, "Initialization should succeed");
    ASSERT_FALSE(grpc_server_is_running(&ctx), "Initialized but not started should not be running");

    result = grpc_server_start(&ctx);
    ASSERT_EQ(0, result, "Start should succeed");

    // Small delay
    struct timespec ts = {0, 100000000};
    nanosleep(&ts, NULL);

    ASSERT_TRUE(grpc_server_is_running(&ctx), "Started server should be running");

    grpc_server_stop(&ctx);
    ASSERT_FALSE(grpc_server_is_running(&ctx), "Stopped server should not be running");

    grpc_server_cleanup(&ctx);

    return 0;
}

/**
 * Test 8: gRPC server multiple start/stop cycles
 */
TEST(test_grpc_server_multiple_cycles) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));

    config.enabled = true;
    config.port = 50057;

    int result = grpc_server_init(&ctx, &config);
    ASSERT_EQ(0, result, "Initialization should succeed");

    // Multiple start/stop cycles
    for (int i = 0; i < 3; i++) {
        result = grpc_server_start(&ctx);
        ASSERT_EQ(0, result, "Start cycle %d should succeed", i);

        struct timespec ts = {0, 50000000}; // 50ms
        nanosleep(&ts, NULL);

        grpc_server_stop(&ctx);
        ASSERT_FALSE(grpc_server_is_running(&ctx), "Should not be running after stop %d", i);
    }

    grpc_server_cleanup(&ctx);

    return 0;
}

/**
 * Test 9: gRPC server null pointer checks
 */
TEST(test_grpc_server_null_checks) {
    grpc_server_config_t config;
    grpc_server_status_t status;
    grpc_server_stats_t stats;

    memset(&config, 0, sizeof(config));

    // Null context checks
    ASSERT_EQ(-1, grpc_server_init(NULL, &config), "Init with NULL ctx should fail");
    ASSERT_EQ(-1, grpc_server_init(&(*(grpc_server_context_t *)0), &config), "Init with NULL should fail");

    grpc_server_stop(NULL);  // Should not crash
    grpc_server_cleanup(NULL);  // Should not crash

    ASSERT_FALSE(grpc_server_is_running(NULL), "is_running with NULL should return false");

    grpc_server_get_stats(NULL, &stats);  // Should not crash
    grpc_server_get_stats((grpc_server_context_t *)0, NULL);  // Should not crash

    grpc_server_reset_stats(NULL);  // Should not crash

    grpc_server_set_status_callback(NULL, NULL);  // Should not crash

    return 0;
}

/**
 * Test 10: gRPC server platform info
 */
TEST(test_grpc_server_platform_info) {
    grpc_server_context_t ctx;
    grpc_server_config_t config;
    grpc_server_status_t status;

    memset(&ctx, 0, sizeof(ctx));
    memset(&config, 0, sizeof(config));
    memset(&status, 0, sizeof(status));

    config.enabled = true;
    config.port = 50058;

    int result = grpc_server_init(&ctx, &config);
    ASSERT_EQ(0, result, "Initialization should succeed");

    result = ctx.get_status_callback(&status);
    ASSERT_EQ(0, result, "get_status_callback should succeed");

    // Check platform info is populated
    ASSERT_TRUE(strlen(status.platform.os) > 0, "OS should not be empty");
    ASSERT_TRUE(strlen(status.platform.arch) > 0, "Arch should not be empty");
    ASSERT_TRUE(status.platform.cpu_cores > 0, "CPU cores should be > 0");
    ASSERT_TRUE(status.platform.total_memory_bytes > 0, "Memory should be > 0");

    printf("    Platform: %s %s, %d cores, %ld bytes RAM\n",
           status.platform.os,
           status.platform.arch,
           status.platform.cpu_cores,
           (long)status.platform.total_memory_bytes);

    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("Running gRPC Server Tests\n");
    printf("=========================\n\n");

    RUN_TEST(test_grpc_server_init);
    RUN_TEST(test_grpc_server_start_stop);
    RUN_TEST(test_grpc_server_status_callback);
    RUN_TEST(test_grpc_server_custom_callback);
    RUN_TEST(test_grpc_server_statistics);
    RUN_TEST(test_grpc_server_state_conversion);
    RUN_TEST(test_grpc_server_is_running);
    RUN_TEST(test_grpc_server_multiple_cycles);
    RUN_TEST(test_grpc_server_null_checks);
    RUN_TEST(test_grpc_server_platform_info);

    printf("\n=========================\n");
    printf("Tests run: %d\n", g_tests_run);
    printf("Passed: %d\n", g_tests_passed);
    printf("Failed: %d\n", g_tests_failed);
    printf("=========================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
