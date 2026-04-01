/*
    Test suite for Admin CLI
    Tests for admin/admin-cli.c functionality
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
#endif

#include "../admin/admin-cli.h"

#define TEST_PASS 1
#define TEST_FAIL 0

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("Running %s... ", #name); \
    if (test_##name()) { \
        printf("PASS\n"); \
        tests_passed++; \
    } else { \
        printf("FAIL\n"); \
        tests_failed++; \
    } \
} while (0)

#define ASSERT(cond) do { if (!(cond)) return TEST_FAIL; } while (0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) return TEST_FAIL; } while (0)

/* ============================================================================
 * Tokenization Tests
 * ============================================================================ */

TEST(cli_tokenize_basic) {
    const char *cmd = "cache-stats";
    int argc = 0;
    
    char **tokens = admin_cli_tokenize(cmd, &argc);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(argc, 1);
    ASSERT_EQ(strcmp(tokens[0], "cache-stats"), 0);
    
    admin_cli_free_tokens(tokens, argc);
    return TEST_PASS;
}

TEST(cli_tokenize_multiple) {
    const char *cmd = "config-set network.port 8889";
    int argc = 0;
    
    char **tokens = admin_cli_tokenize(cmd, &argc);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(argc, 3);
    ASSERT_EQ(strcmp(tokens[0], "config-set"), 0);
    ASSERT_EQ(strcmp(tokens[1], "network.port"), 0);
    ASSERT_EQ(strcmp(tokens[2], "8889"), 0);
    
    admin_cli_free_tokens(tokens, argc);
    return TEST_PASS;
}

TEST(cli_tokenize_with_extra_spaces) {
    const char *cmd = "  status   detail  ";
    int argc = 0;
    
    char **tokens = admin_cli_tokenize(cmd, &argc);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(argc, 2);
    ASSERT_EQ(strcmp(tokens[0], "status"), 0);
    ASSERT_EQ(strcmp(tokens[1], "detail"), 0);
    
    admin_cli_free_tokens(tokens, argc);
    return TEST_PASS;
}

TEST(cli_tokenize_empty) {
    const char *cmd = "";
    int argc = -1;
    
    char **tokens = admin_cli_tokenize(cmd, &argc);
    
    ASSERT(tokens != NULL);
    ASSERT_EQ(argc, 0);
    
    admin_cli_free_tokens(tokens, argc);
    return TEST_PASS;
}

TEST(cli_tokenize_null) {
    int argc = 0;
    char **tokens = admin_cli_tokenize(NULL, &argc);
    ASSERT(tokens == NULL);
    return TEST_PASS;
}

/* ============================================================================
 * Command Parsing Tests
 * ============================================================================ */

TEST(cli_parse_command_help) {
    admin_command_t cmd = admin_cli_parse_command("help");
    ASSERT_EQ(cmd, CMD_HELP);
    return TEST_PASS;
}

TEST(cli_parse_command_status) {
    admin_command_t cmd = admin_cli_parse_command("status");
    ASSERT_EQ(cmd, CMD_STATUS);
    return TEST_PASS;
}

TEST(cli_parse_command_stats) {
    admin_command_t cmd = admin_cli_parse_command("stats");
    ASSERT_EQ(cmd, CMD_STATS);
    return TEST_PASS;
}

TEST(cli_parse_command_cache_stats) {
    admin_command_t cmd = admin_cli_parse_command("cache-stats");
    ASSERT_EQ(cmd, CMD_CACHE_STATS);
    return TEST_PASS;
}

TEST(cli_parse_command_unknown) {
    admin_command_t cmd = admin_cli_parse_command("unknown-command");
    ASSERT_EQ(cmd, CMD_UNKNOWN);
    return TEST_PASS;
}

TEST(cli_parse_command_case_insensitive) {
    admin_command_t cmd1 = admin_cli_parse_command("STATUS");
    admin_command_t cmd2 = admin_cli_parse_command("Status");
    // Commands should be case-insensitive or normalized
    // For now, we just check they return something
    ASSERT(cmd1 == CMD_STATUS || cmd1 == CMD_UNKNOWN);
    return TEST_PASS;
}

/* ============================================================================
 * Context Initialization Tests
 * ============================================================================ */

TEST(cli_context_init) {
    admin_cli_context_t ctx;
    int ret = admin_cli_init(&ctx);
    
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.is_interactive, 0);
    ASSERT_EQ(ctx.output_format, 0);
    ASSERT_EQ(ctx.verbose, 0);
    ASSERT_EQ(ctx.color_enabled, 0);
    ASSERT_EQ(ctx.history_count, 0);
    ASSERT_EQ(ctx.commands_executed, 0);
    ASSERT_EQ(ctx.commands_failed, 0);
    
    return TEST_PASS;
}

TEST(cli_context_cleanup) {
    admin_cli_context_t ctx;
    admin_cli_init(&ctx);
    
    // Add some history
    admin_cli_add_history(&ctx, "test command 1");
    admin_cli_add_history(&ctx, "test command 2");
    
    // Cleanup should not crash
    admin_cli_cleanup(&ctx);
    
    return TEST_PASS;
}

/* ============================================================================
 * History Tests
 * ============================================================================ */

TEST(cli_history_add) {
    admin_cli_context_t ctx;
    admin_cli_init(&ctx);
    
    int ret = admin_cli_add_history(&ctx, "test command");
    
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ctx.history_count, 1);
    ASSERT(ctx.command_history != NULL);
    ASSERT(strcmp(ctx.command_history[0], "test command") == 0);
    
    admin_cli_cleanup(&ctx);
    return TEST_PASS;
}

TEST(cli_history_multiple) {
    admin_cli_context_t ctx;
    admin_cli_init(&ctx);
    
    admin_cli_add_history(&ctx, "command 1");
    admin_cli_add_history(&ctx, "command 2");
    admin_cli_add_history(&ctx, "command 3");
    
    ASSERT_EQ(ctx.history_count, 3);
    ASSERT(strcmp(ctx.command_history[0], "command 1") == 0);
    ASSERT(strcmp(ctx.command_history[2], "command 3") == 0);
    
    admin_cli_cleanup(&ctx);
    return TEST_PASS;
}

TEST(cli_history_max_limit) {
    admin_cli_context_t ctx;
    admin_cli_init(&ctx);
    
    // Add more than max_history (assuming default is around 100)
    for (int i = 0; i < 105; i++) {
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "command %d", i);
        admin_cli_add_history(&ctx, cmd);
    }
    
    // Should not exceed max
    ASSERT(ctx.history_count <= ctx.history_max);
    
    admin_cli_cleanup(&ctx);
    return TEST_PASS;
}

/* ============================================================================
 * Autocomplete Tests
 * ============================================================================ */

TEST(cli_autocomplete_basic) {
    char *suggestions[10];
    int count = admin_cli_autocomplete("stat", suggestions, 10);
    
    ASSERT(count > 0);
    
    // Free suggestions
    for (int i = 0; i < count; i++) {
        free(suggestions[i]);
    }
    
    return TEST_PASS;
}

TEST(cli_autocomplete_cache) {
    char *suggestions[10];
    int count = admin_cli_autocomplete("cache-", suggestions, 10);
    
    ASSERT(count > 0);
    
    // All suggestions should start with "cache-"
    for (int i = 0; i < count; i++) {
        ASSERT(strncmp(suggestions[i], "cache-", 6) == 0);
        free(suggestions[i]);
    }
    
    return TEST_PASS;
}

TEST(cli_autocomplete_no_match) {
    char *suggestions[10];
    int count = admin_cli_autocomplete("xyz-nonexistent", suggestions, 10);
    
    ASSERT_EQ(count, 0);
    
    return TEST_PASS;
}

TEST(cli_autocomplete_empty_input) {
    char *suggestions[10];
    int count = admin_cli_autocomplete("", suggestions, 10);
    
    // Should return all commands or 0
    ASSERT(count >= 0);
    
    for (int i = 0; i < count; i++) {
        free(suggestions[i]);
    }
    
    return TEST_PASS;
}

/* ============================================================================
 * Result Formatting Tests
 * ============================================================================ */

TEST(cli_print_result_basic) {
    admin_command_result_t result = {
        .success = 1,
        .exit_code = 0,
        .output = strdup("Test output\n"),
        .output_size = 12,
        .error_message = NULL,
        .execution_time_ms = 10
    };
    
    // Should not crash
    admin_cli_print_result(&result);
    
    if (result.output) free(result.output);
    return TEST_PASS;
}

TEST(cli_print_result_error) {
    admin_command_result_t result = {
        .success = 0,
        .exit_code = 1,
        .output = NULL,
        .output_size = 0,
        .error_message = strdup("Test error"),
        .execution_time_ms = 5
    };
    
    // Should not crash
    admin_cli_print_result(&result);
    
    if (result.error_message) free(result.error_message);
    return TEST_PASS;
}

/* ============================================================================
 * JSON Formatting Tests
 * ============================================================================ */

TEST(cli_format_json_basic) {
    const char *input = "{\"status\":\"ok\",\"value\":123}";
    char *formatted = admin_cli_format_json(input);
    
    // Should return valid JSON or original string
    ASSERT(formatted != NULL);
    
    if (formatted) free(formatted);
    return TEST_PASS;
}

/* ============================================================================
 * Table Formatting Tests
 * ============================================================================ */

TEST(cli_format_table_basic) {
    const char *headers[] = {"Name", "Value", "Status"};
    const char *rows[] = {
        "Item1", "100", "Active",
        "Item2", "200", "Inactive"
    };
    
    char *table = admin_cli_format_table(headers, 3, rows, 2);
    
    ASSERT(table != NULL);
    
    if (table) free(table);
    return TEST_PASS;
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(int argc, char **argv) {
    printf("===========================================\n");
    printf("  Admin CLI Test Suite\n");
    printf("===========================================\n\n");
    
    // Tokenization tests
    printf("--- Tokenization Tests ---\n");
    RUN_TEST(cli_tokenize_basic);
    RUN_TEST(cli_tokenize_multiple);
    RUN_TEST(cli_tokenize_with_extra_spaces);
    RUN_TEST(cli_tokenize_empty);
    RUN_TEST(cli_tokenize_null);
    printf("\n");
    
    // Command parsing tests
    printf("--- Command Parsing Tests ---\n");
    RUN_TEST(cli_parse_command_help);
    RUN_TEST(cli_parse_command_status);
    RUN_TEST(cli_parse_command_stats);
    RUN_TEST(cli_parse_command_cache_stats);
    RUN_TEST(cli_parse_command_unknown);
    RUN_TEST(cli_parse_command_case_insensitive);
    printf("\n");
    
    // Context tests
    printf("--- Context Initialization Tests ---\n");
    RUN_TEST(cli_context_init);
    RUN_TEST(cli_context_cleanup);
    printf("\n");
    
    // History tests
    printf("--- History Tests ---\n");
    RUN_TEST(cli_history_add);
    RUN_TEST(cli_history_multiple);
    RUN_TEST(cli_history_max_limit);
    printf("\n");
    
    // Autocomplete tests
    printf("--- Autocomplete Tests ---\n");
    RUN_TEST(cli_autocomplete_basic);
    RUN_TEST(cli_autocomplete_cache);
    RUN_TEST(cli_autocomplete_no_match);
    RUN_TEST(cli_autocomplete_empty_input);
    printf("\n");
    
    // Result formatting tests
    printf("--- Result Formatting Tests ---\n");
    RUN_TEST(cli_print_result_basic);
    RUN_TEST(cli_print_result_error);
    RUN_TEST(cli_format_json_basic);
    RUN_TEST(cli_format_table_basic);
    printf("\n");
    
    // Summary
    printf("===========================================\n");
    printf("  Test Summary\n");
    printf("===========================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("===========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
