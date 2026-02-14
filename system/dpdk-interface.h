/*
 * Интерфейс DPDK для MTProxy
 * Поддержка высокопроизводительной работы с сетью через userspace networking
 */

#ifndef _DPDK_INTERFACE_H_
#define _DPDK_INTERFACE_H_

#include <stdint.h>

// Статус DPDK
typedef enum {
    DPDK_STATUS_UNINITIALIZED = 0,
    DPDK_STATUS_INITIALIZED = 1,
    DPDK_STATUS_RUNNING = 2,
    DPDK_STATUS_ERROR = 3
} dpdk_status_t;

// Типы пакетов
typedef enum {
    DPDK_PKT_TYPE_UNKNOWN = 0,
    DPDK_PKT_TYPE_TCP = 1,
    DPDK_PKT_TYPE_UDP = 2,
    DPDK_PKT_TYPE_ICMP = 3
} dpdk_packet_type_t;

// Статистика DPDK
typedef struct {
    long long packets_received;
    long long packets_sent;
    long long bytes_received;
    long long bytes_sent;
    long long packets_dropped;
    long long errors;
    long long allocation_failures;
    int current_lcore;
    int total_lcores;
    int active_ports;
} dpdk_stats_t;

// Конфигурация DPDK
typedef struct {
    int enable_dpdk;
    int num_lcores;
    int num_mbufs;
    int mbuf_cache_size;
    int rx_desc;
    int tx_desc;
    int enable_promiscuous;
    int enable_jumbo_frames;
    int jumbo_frame_max_size;
    char *memory_channels;
    char *hugepage_size;
} dpdk_config_t;

// Контекст DPDK
typedef struct {
    dpdk_config_t config;
    dpdk_stats_t stats;
    dpdk_status_t status;
    int dpdk_available;
    int max_ports;
    int initialized_ports;
    void *mbuf_pool;
    void *rx_queues;
    void *tx_queues;
} dpdk_context_t;

// Структура для пакета
typedef struct {
    void *data;
    size_t length;
    size_t max_length;
    dpdk_packet_type_t type;
    int port_id;
    int queue_id;
    int vlan_id;
    int timestamp;
    int valid;
} dpdk_packet_t;

// Структура для описания порта
typedef struct {
    int port_id;
    char mac_address[18];  // "XX:XX:XX:XX:XX:XX"
    int mtu;
    int link_speed;
    int link_status;  // 1 = up, 0 = down
    int num_rx_queues;
    int num_tx_queues;
    int promiscuous_enabled;
} dpdk_port_info_t;

// Функции инициализации
int dpdk_init(dpdk_context_t *ctx);
int dpdk_init_with_config(dpdk_context_t *ctx, const dpdk_config_t *config);
void dpdk_cleanup(dpdk_context_t *ctx);
int dpdk_init_eal(int argc, char *argv[]);

// Функции управления портами
int dpdk_init_port(int port_id, dpdk_port_info_t *port_info);
int dpdk_start_port(int port_id);
int dpdk_stop_port(int port_id);
int dpdk_close_port(int port_id);
int dpdk_get_port_info(int port_id, dpdk_port_info_t *port_info);

// Функции работы с пакетами
int dpdk_receive_packet(dpdk_context_t *ctx, int port_id, int queue_id, dpdk_packet_t *packet);
int dpdk_transmit_packet(dpdk_context_t *ctx, int port_id, int queue_id, dpdk_packet_t *packet);
int dpdk_allocate_packet(dpdk_context_t *ctx, dpdk_packet_t *packet, size_t size);
int dpdk_free_packet(dpdk_context_t *ctx, dpdk_packet_t *packet);
int dpdk_batch_receive_packets(dpdk_context_t *ctx, int port_id, int queue_id, 
                               dpdk_packet_t *packets, int max_packets);
int dpdk_batch_transmit_packets(dpdk_context_t *ctx, int port_id, int queue_id, 
                                dpdk_packet_t *packets, int num_packets);

// Функции управления очередями
int dpdk_setup_rx_queue(int port_id, int queue_id, int ring_size);
int dpdk_setup_tx_queue(int port_id, int queue_id, int ring_size);
int dpdk_enable_rx_interrupt(int port_id, int queue_id);
int dpdk_disable_rx_interrupt(int port_id, int queue_id);

// Функции статистики
dpdk_stats_t dpdk_get_stats(dpdk_context_t *ctx);
void dpdk_reset_stats(dpdk_context_t *ctx);
int dpdk_get_port_stats(int port_id, void *stats);  // stats будет структурой rte_eth_stats

// Функции конфигурации
void dpdk_get_config(dpdk_context_t *ctx, dpdk_config_t *config);
int dpdk_update_config(dpdk_context_t *ctx, const dpdk_config_t *new_config);

// Вспомогательные функции
int dpdk_is_available(void);
int dpdk_get_num_lcores(void);
int dpdk_get_num_ports(void);
int dpdk_get_port_mac_address(int port_id, char *mac_addr, int mac_addr_len);
int dpdk_set_port_mac_address(int port_id, const char *mac_addr);
int dpdk_enable_promiscuous_mode(int port_id);
int dpdk_disable_promiscuous_mode(int port_id);

#endif