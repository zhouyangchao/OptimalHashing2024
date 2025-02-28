#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// Define hash function type
typedef uint32_t (*hash_function)(const void* key, size_t key_size, uint32_t attempt);

/**
 * @brief Structure containing hash table statistics
 */
typedef struct {
    uint64_t total_probes;    // 总探测次数
    uint64_t num_operations;  // 操作总数
    double avg_probes;        // 平均探测次数
    size_t num_entries;       // 当前条目数
    size_t table_size;        // 表大小
    uint32_t max_probes;      // 最大探测次数
    uint32_t probe_dist[10];  // 探测分布

    // 按操作类型细分的探测次数
    uint64_t insert_probes;   // 插入操作的探测次数
    uint64_t lookup_probes;   // 查找操作的探测次数
    uint64_t delete_probes;   // 删除操作的探测次数

    // 操作次数统计
    uint64_t insert_ops;      // 插入操作次数
    uint64_t lookup_ops;      // 查找操作次数
    uint64_t delete_ops;      // 删除操作次数
} hash_stats_t;

// Generic key-value pair structure
typedef struct {
    void* key;
    size_t key_size;
    void* value;
    size_t value_size;
    bool occupied;
} hash_entry_t;

// Utility functions
uint32_t default_hash(const void* key, size_t key_size, uint32_t attempt);
void init_hash_stats(hash_stats_t* stats);
void update_hash_stats(hash_stats_t* stats, uint32_t probes);
void print_hash_stats(const hash_stats_t* stats);

// Helper macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Memory management helpers
void* safe_malloc(size_t size);
void* safe_calloc(size_t nmemb, size_t size);
void* safe_realloc(void* ptr, size_t size);

#endif /* COMMON_H */