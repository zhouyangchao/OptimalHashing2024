#include "../include/common.h"

// A simple default hash function based on djb2
uint32_t default_hash(const void* key, size_t key_size, uint32_t attempt) {
    const unsigned char* str = (const unsigned char*)key;
    uint32_t hash = 5381;

    for (size_t i = 0; i < key_size; i++) {
        hash = ((hash << 5) + hash) + str[i]; // hash * 33 + c
    }

    // For subsequent attempts, use double hashing
    if (attempt > 0) {
        uint32_t h2 = 1 + (hash % (UINT32_MAX - 1));
        hash = hash + attempt * h2;
    }

    return hash;
}

// Initialize hash table statistics
void init_hash_stats(hash_stats_t* stats) {
    if (!stats) return;

    stats->total_probes = 0;
    stats->num_operations = 0;
    stats->avg_probes = 0.0;
    stats->num_entries = 0;
    stats->table_size = 0;
    stats->insert_probes = 0;
    stats->lookup_probes = 0;
    stats->delete_probes = 0;
    stats->insert_ops = 0;
    stats->lookup_ops = 0;
    stats->delete_ops = 0;
    stats->max_probes = 0;
    for (int i = 0; i < 10; i++) {
        stats->probe_dist[i] = 0;
    }
    // 初始化其他统计字段
}

// Update statistics after an operation
void update_hash_stats(hash_stats_t* stats, uint32_t probes) {
    if (!stats) return;

    stats->total_probes += probes;
    stats->num_operations++;

    // 更新平均探测次数
    stats->avg_probes = (double)stats->total_probes / stats->num_operations;

    // 更新最大探测次数
    if (probes > stats->max_probes) {
        stats->max_probes = probes;
    }

    // 更新探测分布
    if (probes == 0) stats->probe_dist[0]++;
    else if (probes == 1) stats->probe_dist[1]++;
    else if (probes == 2) stats->probe_dist[2]++;
    else if (probes == 3) stats->probe_dist[3]++;
    else if (probes == 4) stats->probe_dist[4]++;
    else if (probes <= 8) stats->probe_dist[5]++;
    else if (probes <= 16) stats->probe_dist[6]++;
    else if (probes <= 32) stats->probe_dist[7]++;
    else if (probes <= 64) stats->probe_dist[8]++;
    else stats->probe_dist[9]++;
}

// Print statistics
void print_hash_stats(const hash_stats_t* stats) {
    if (!stats) return;

    printf("Hash Table Statistics:\n");
    printf("Total entries: %zu\n", stats->num_entries);
    printf("Table size: %zu\n", stats->table_size);
    printf("Total probes: %llu\n", (unsigned long long)stats->total_probes);
    printf("Average probe count: %.2f\n", stats->avg_probes);
    printf("Current load factor: %.2f\n", (double)stats->num_entries / stats->table_size);
    printf("Max probes: %u\n", stats->max_probes);
    printf("Operation count: %llu\n", (unsigned long long)stats->num_operations);
    printf("Insert operations: %llu (avg probes: %.2f)\n",
           (unsigned long long)stats->insert_ops,
           stats->insert_ops > 0 ? (double)stats->insert_probes / stats->insert_ops : 0.0);
    printf("Lookup operations: %llu (avg probes: %.2f)\n",
           (unsigned long long)stats->lookup_ops,
           stats->lookup_ops > 0 ? (double)stats->lookup_probes / stats->lookup_ops : 0.0);
    printf("Delete operations: %llu (avg probes: %.2f)\n",
           (unsigned long long)stats->delete_ops,
           stats->delete_ops > 0 ? (double)stats->delete_probes / stats->delete_ops : 0.0);
    printf("Probe distribution:\n");
    for (int i = 0; i < 10; i++) {
        printf("%d-%d: %u\n", i * 5, (i + 1) * 5 - 1, stats->probe_dist[i]);
    }
}

// Safe memory allocation functions
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safe_calloc(size_t nmemb, size_t size) {
    void* ptr = calloc(nmemb, size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void* safe_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "Memory reallocation failed\n");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}