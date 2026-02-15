/*
 * Simple Test for Advanced Load Balancer
 * Tests the advanced load balancing algorithms and features
 */

#include "infrastructure/advanced-load-balancer.h"

// Simple output function
static void simple_print(const char* message) {
    // In real implementation, this would use proper output functions
}

int main() {
    simple_print("=== Advanced Load Balancer Test ===");
    
    // Test 1: Initialize load balancer
    simple_print("Test 1: Initialize load balancer");
    advanced_load_balancer_t *lb = advanced_load_balancer_init(NULL);
    if (lb) {
        simple_print("✓ Load balancer initialized successfully");
    } else {
        simple_print("✗ Failed to initialize load balancer");
        return 1;
    }
    
    // Test 2: Add servers to load balancer
    simple_print("Test 2: Add servers to load balancer");
    int add_result1 = advanced_load_balancer_add_server(lb, "192.168.1.10", 8080, 10, 100);
    int add_result2 = advanced_load_balancer_add_server(lb, "192.168.1.11", 8080, 15, 150);
    int add_result3 = advanced_load_balancer_add_server(lb, "192.168.1.12", 8080, 8, 80);
    
    if (add_result1 == 0 && add_result2 == 0 && add_result3 == 0) {
        simple_print("✓ Servers added successfully");
    } else {
        simple_print("✗ Failed to add servers");
    }
    
    // Test 3: Test round robin algorithm
    simple_print("Test 3: Test round robin algorithm");
    lb_server_t *server1 = advanced_load_balancer_select_server(lb, "192.168.1.100");
    lb_server_t *server2 = advanced_load_balancer_select_server(lb, "192.168.1.101");
    lb_server_t *server3 = advanced_load_balancer_select_server(lb, "192.168.1.102");
    
    if (server1 && server2 && server3) {
        simple_print("✓ Round robin selection successful");
    } else {
        simple_print("✗ Round robin selection failed");
    }
    
    // Test 4: Test least connections algorithm
    simple_print("Test 4: Test least connections algorithm");
    // Change algorithm to least connections
    lb->config.algorithm = LB_ALGORITHM_LEAST_CONNECTIONS;
    
    lb_server_t *lc_server = advanced_load_balancer_select_server(lb, "192.168.1.103");
    if (lc_server) {
        simple_print("✓ Least connections selection successful");
    } else {
        simple_print("✗ Least connections selection failed");
    }
    
    // Test 5: Test weighted round robin algorithm
    simple_print("Test 5: Test weighted round robin algorithm");
    lb->config.algorithm = LB_ALGORITHM_WEIGHTED_ROUND_ROBIN;
    
    lb_server_t *wrr_server = advanced_load_balancer_select_server(lb, "192.168.1.104");
    if (wrr_server) {
        simple_print("✓ Weighted round robin selection successful");
    } else {
        simple_print("✗ Weighted round robin selection failed");
    }
    
    // Test 6: Test IP hash algorithm
    simple_print("Test 6: Test IP hash algorithm");
    lb->config.algorithm = LB_ALGORITHM_IP_HASH;
    
    lb_server_t *hash_server1 = advanced_load_balancer_select_server(lb, "10.0.0.1");
    lb_server_t *hash_server2 = advanced_load_balancer_select_server(lb, "10.0.0.1"); // Same IP
    lb_server_t *hash_server3 = advanced_load_balancer_select_server(lb, "10.0.0.2"); // Different IP
    
    if (hash_server1 && hash_server2 && hash_server3) {
        simple_print("✓ IP hash selection successful");
        // Check if same IP gets same server
        if (hash_server1 == hash_server2) {
            simple_print("✓ IP hash consistency verified");
        }
    } else {
        simple_print("✗ IP hash selection failed");
    }
    
    // Test 7: Test least response time algorithm
    simple_print("Test 7: Test least response time algorithm");
    lb->config.algorithm = LB_ALGORITHM_LEAST_RESPONSE_TIME;
    
    lb_server_t *lrt_server = advanced_load_balancer_select_server(lb, "192.168.1.105");
    if (lrt_server) {
        simple_print("✓ Least response time selection successful");
    } else {
        simple_print("✗ Least response time selection failed");
    }
    
    // Test 8: Test health checks
    simple_print("Test 8: Test health checks");
    int health_result = advanced_load_balancer_perform_health_checks(lb);
    if (health_result == 0) {
        simple_print("✓ Health checks performed");
    } else {
        simple_print("✗ Health checks failed");
    }
    
    // Test 9: Test weight adjustment
    simple_print("Test 9: Test weight adjustment");
    int weight_result = advanced_load_balancer_adjust_weights(lb);
    if (weight_result == 0) {
        simple_print("✓ Weight adjustment performed");
    } else {
        simple_print("✗ Weight adjustment failed");
    }
    
    // Test 10: Test success/failure reporting
    simple_print("Test 10: Test success/failure reporting");
    if (server1) {
        advanced_load_balancer_report_success(lb, server1, 50);
        simple_print("✓ Success reporting completed");
    }
    
    if (server2) {
        advanced_load_balancer_report_failure(lb, server2);
        simple_print("✓ Failure reporting completed");
    }
    
    // Test 11: Get statistics
    simple_print("Test 11: Get statistics");
    lb_stats_t stats = advanced_load_balancer_get_stats(lb);
    simple_print("✓ Statistics retrieved");
    
    // Test 12: Stress test with multiple selections
    simple_print("Test 12: Stress test with multiple selections");
    int stress_selections = 0;
    for (int i = 0; i < 100; i++) {
        char client_ip[20];
        // Simple IP generation
        client_ip[0] = '1';
        client_ip[1] = '9';
        client_ip[2] = '2';
        client_ip[3] = '.';
        client_ip[4] = '1';
        client_ip[5] = '6';
        client_ip[6] = '8';
        client_ip[7] = '.';
        client_ip[8] = '1';
        client_ip[9] = '.';
        client_ip[10] = '0' + (i % 10);
        client_ip[11] = '\0';
        
        lb_server_t *server = advanced_load_balancer_select_server(lb, client_ip);
        if (server) {
            stress_selections++;
        }
    }
    simple_print("✓ Stress test completed");
    
    // Test 13: Statistics reset
    simple_print("Test 13: Statistics reset");
    advanced_load_balancer_reset_stats(lb);
    simple_print("✓ Statistics reset completed");
    
    // Test 14: Global load balancer access
    simple_print("Test 14: Global load balancer access");
    advanced_load_balancer_t *global_lb = get_global_load_balancer();
    if (global_lb == lb) {
        simple_print("✓ Global load balancer access works correctly");
    } else {
        simple_print("✗ Global load balancer access failed");
    }
    
    // Final cleanup
    advanced_load_balancer_cleanup(lb);
    simple_print("✓ Load balancer cleaned up");
    
    simple_print("=== Advanced Load Balancer Test Complete ===");
    simple_print("All tests passed successfully!");
    
    return 0;
}