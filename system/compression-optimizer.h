/*
 * compression-optimizer.h
 * Advanced Compression Optimization System
 *
 * This system provides intelligent compression optimization for network traffic,
 * automatically selecting the best compression algorithms and parameters based
 * on data characteristics and performance requirements.
 */

#ifndef COMPRESSION_OPTIMIZER_H
#define COMPRESSION_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compression algorithms
typedef enum {
    COMPRESSION_ALGO_NONE = 0,
    COMPRESSION_ALGO_LZ4,
    COMPRESSION_ALGO_LZ4_HC,
    COMPRESSION_ALGO_ZSTD,
    COMPRESSION_ALGO_ZLIB,
    COMPRESSION_ALGO_GZIP,
    COMPRESSION_ALGO_BROTLI,
    COMPRESSION_ALGO_SNAPPY
} compression_algo_t;

// Data types for compression analysis
typedef enum {
    DATA_TYPE_UNKNOWN = 0,
    DATA_TYPE_TEXT,
    DATA_TYPE_BINARY,
    DATA_TYPE_JSON,
    DATA_TYPE_XML,
    DATA_TYPE_IMAGE,
    DATA_TYPE_AUDIO,
    DATA_TYPE_VIDEO,
    DATA_TYPE_ENCRYPTED
} data_type_t;

// Compression optimization modes
typedef enum {
    COMPRESSION_MODE_SPEED = 0,
    COMPRESSION_MODE_COMPRESSION_RATIO,
    COMPRESSION_MODE_BALANCED,
    COMPRESSION_MODE_ADAPTIVE
} compression_mode_t;

// Compression context structure
typedef struct {
    compression_algo_t algorithm;
    int compression_level;
    data_type_t data_type;
    double compression_ratio;
    double compression_speed_mbps;
    double decompression_speed_mbps;
    uint64_t data_size_original;
    uint64_t data_size_compressed;
    uint64_t compression_time_us;
    uint64_t decompression_time_us;
} compression_result_t;

// Compression optimizer configuration
typedef struct {
    int enable_compression_optimization;
    compression_mode_t default_mode;
    int auto_detect_data_type;
    int enable_adaptive_compression;
    int min_data_size_for_compression;
    double min_compression_ratio_threshold;
    int compression_level_range_min;
    int compression_level_range_max;
    bool enable_compression_caching;
    int cache_size_limit_mb;
    bool enable_parallel_compression;
    int max_parallel_threads;
    bool enable_compression_preprocessing;
    int preprocessing_window_size;
} compression_config_t;

// Performance statistics
typedef struct {
    uint64_t total_compression_operations;
    uint64_t successful_compressions;
    uint64_t failed_compressions;
    uint64_t total_data_processed_bytes;
    uint64_t total_compressed_bytes;
    double average_compression_ratio;
    double average_compression_speed_mbps;
    double average_decompression_speed_mbps;
    uint64_t cache_hits;
    uint64_t cache_misses;
    compression_algo_t most_used_algorithm;
    compression_algo_t best_performance_algorithm;
} compression_stats_t;

// Compression optimizer context
typedef struct {
    compression_config_t config;
    compression_stats_t stats;
    compression_result_t* compression_history;
    int history_count;
    compression_result_t* compression_cache;
    int cache_count;
    uint64_t last_optimization_time;
    bool is_optimizing;
    compression_algo_t current_algorithm;
    int current_compression_level;
    void* compression_contexts[8];  // Contexts for different algorithms
} compression_optimizer_ctx_t;

// Data analysis structure
typedef struct {
    data_type_t detected_type;
    double compressibility_score;  // 0.0 - 1.0 (higher = more compressible)
    double entropy;
    double redundancy_ratio;
    int recommended_compression_level;
    compression_algo_t recommended_algorithm;
    char analysis_notes[256];
} data_analysis_result_t;

// Callback function types
typedef void (*compression_result_callback_t)(const compression_result_t* result);
typedef void (*algorithm_switch_callback_t)(compression_algo_t old_algo, compression_algo_t new_algo);
typedef void (*performance_update_callback_t)(const compression_stats_t* stats);

// Function prototypes

// Initialization and cleanup
int init_compression_optimizer(compression_optimizer_ctx_t* ctx);
int init_compression_optimizer_with_config(compression_optimizer_ctx_t* ctx, const compression_config_t* config);
void cleanup_compression_optimizer(compression_optimizer_ctx_t* ctx);

// Configuration management
void get_compression_config(compression_optimizer_ctx_t* ctx, compression_config_t* config);
int set_compression_config(compression_optimizer_ctx_t* ctx, const compression_config_t* config);

// Core compression functions
compression_result_t compress_data(compression_optimizer_ctx_t* ctx, 
                                  const uint8_t* input_data, 
                                  size_t input_size,
                                  uint8_t* output_buffer, 
                                  size_t output_buffer_size);
compression_result_t decompress_data(compression_optimizer_ctx_t* ctx,
                                    const uint8_t* compressed_data,
                                    size_t compressed_size,
                                    uint8_t* output_buffer,
                                    size_t output_buffer_size);
int get_compression_buffer_size(compression_optimizer_ctx_t* ctx, size_t input_size);

// Data analysis and optimization
data_analysis_result_t analyze_data_compressibility(const uint8_t* data, size_t data_size);
compression_algo_t select_optimal_algorithm(compression_optimizer_ctx_t* ctx, 
                                          const data_analysis_result_t* analysis);
int select_optimal_compression_level(compression_optimizer_ctx_t* ctx,
                                   compression_algo_t algorithm,
                                   const data_analysis_result_t* analysis);
int optimize_compression_parameters(compression_optimizer_ctx_t* ctx,
                                  const uint8_t* sample_data,
                                  size_t sample_size);

// Adaptive compression
int enable_adaptive_compression(compression_optimizer_ctx_t* ctx);
int disable_adaptive_compression(compression_optimizer_ctx_t* ctx);
bool is_adaptive_compression_enabled(compression_optimizer_ctx_t* ctx);
int update_compression_strategy(compression_optimizer_ctx_t* ctx,
                               const compression_result_t* recent_results);

// Caching functions
int enable_compression_caching(compression_optimizer_ctx_t* ctx);
int disable_compression_caching(compression_optimizer_ctx_t* ctx);
compression_result_t* lookup_compression_cache(compression_optimizer_ctx_t* ctx,
                                             const uint8_t* data_hash,
                                             size_t data_size);
int add_to_compression_cache(compression_optimizer_ctx_t* ctx,
                           const uint8_t* data_hash,
                           size_t data_size,
                           const compression_result_t* result);
int clear_compression_cache(compression_optimizer_ctx_t* ctx);

// Performance monitoring
double get_current_compression_ratio(compression_optimizer_ctx_t* ctx);
double get_average_compression_speed(compression_optimizer_ctx_t* ctx);
double get_average_decompression_speed(compression_optimizer_ctx_t* ctx);
compression_stats_t get_compression_statistics(compression_optimizer_ctx_t* ctx);
void reset_compression_statistics(compression_optimizer_ctx_t* ctx);

// Algorithm management
int register_compression_algorithm(compression_optimizer_ctx_t* ctx,
                                 compression_algo_t algorithm,
                                 void* algorithm_context);
int unregister_compression_algorithm(compression_optimizer_ctx_t* ctx,
                                   compression_algo_t algorithm);
bool is_algorithm_supported(compression_optimizer_ctx_t* ctx, compression_algo_t algorithm);
const char* get_algorithm_name(compression_algo_t algorithm);
compression_algo_t get_algorithm_by_name(const char* name);

// Utility functions
const char* compression_algo_to_string(compression_algo_t algo);
const char* data_type_to_string(data_type_t type);
const char* compression_mode_to_string(compression_mode_t mode);
double calculate_entropy(const uint8_t* data, size_t data_size);
double calculate_redundancy(const uint8_t* data, size_t data_size);
bool is_data_compressible(const data_analysis_result_t* analysis);
int generate_data_hash(const uint8_t* data, size_t data_size, uint8_t* hash_output);

// Batch processing
int compress_batch_data(compression_optimizer_ctx_t* ctx,
                       const uint8_t** input_data_array,
                       const size_t* input_sizes,
                       int data_count,
                       uint8_t** output_buffers,
                       size_t* output_sizes);
int decompress_batch_data(compression_optimizer_ctx_t* ctx,
                         const uint8_t** compressed_data_array,
                         const size_t* compressed_sizes,
                         int data_count,
                         uint8_t** output_buffers,
                         size_t* output_sizes);

// Memory management
int set_compression_memory_limit(compression_optimizer_ctx_t* ctx, size_t memory_limit_bytes);
size_t get_current_memory_usage(compression_optimizer_ctx_t* ctx);
int optimize_memory_allocation(compression_optimizer_ctx_t* ctx);

// Callback registration
void register_compression_result_callback(compression_result_callback_t callback);
void register_algorithm_switch_callback(algorithm_switch_callback_t callback);
void register_performance_update_callback(performance_update_callback_t callback);

// Integration functions
int integrate_with_network_layer(compression_optimizer_ctx_t* ctx);
int integrate_with_file_system(compression_optimizer_ctx_t* ctx);
int apply_compression_optimizations(compression_optimizer_ctx_t* ctx);
int verify_compression_integrity(compression_optimizer_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // COMPRESSION_OPTIMIZER_H