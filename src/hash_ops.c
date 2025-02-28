/**
 * @file hash_ops.c
 * @brief Implementation of unified hash table operations
 */

#include <stdlib.h>
#include <string.h>
#include "../include/hash_ops.h"
#include "../include/elastic_hash.h"
#include "../include/funnel_hash.h"
#include "../include/linear_hash.h"
#include "../include/uniform_hash.h"

// Get the operations structure for a specific hash table type
const hash_ops_t* get_hash_ops(hash_type_t type) {
    switch (type) {
        case HASH_TYPE_ELASTIC:
            return elastic_hash_get_ops();
        case HASH_TYPE_FUNNEL:
            return funnel_hash_get_ops();
        case HASH_TYPE_LINEAR:
            return linear_hash_get_ops();
        case HASH_TYPE_UNIFORM:
            return uniform_hash_get_ops();
        default:
            return NULL;
    }
}

// Create a generic hash table
generic_hash_t* hash_create(hash_type_t type, size_t size, double param,
                           hash_function hash_func) {
    generic_hash_t* hash = (generic_hash_t*)safe_malloc(sizeof(generic_hash_t));
    const hash_ops_t* ops = NULL;
    void* table = NULL;

    // Select appropriate operations and create table
    switch (type) {
        case HASH_TYPE_ELASTIC:
            ops = elastic_hash_get_ops();
            // For elastic hash, param is the delta error parameter
            table = ops->create(size, param, hash_func);
            break;
        case HASH_TYPE_FUNNEL:
            ops = funnel_hash_get_ops();
            // For funnel hash, param is the delta error parameter
            table = ops->create(size, param, hash_func);
            break;
        case HASH_TYPE_LINEAR:
            ops = linear_hash_get_ops();
            // For linear hash, param is the max load factor
            table = ops->create(size, param, hash_func);
            break;
        case HASH_TYPE_UNIFORM:
            ops = uniform_hash_get_ops();
            // For uniform hash, param is the max load factor
            table = ops->create(size, param, hash_func);
            break;
        default:
            free(hash);
            return NULL;
    }

    if (!table) {
        free(hash);
        return NULL;
    }

    hash->ops = ops;
    hash->table = table;
    return hash;
}

// Insert a key-value pair into the generic hash table
bool hash_insert(generic_hash_t* hash, const void* key, size_t key_size,
                const void* value, size_t value_size) {
    if (!hash || !hash->ops) {
        return false;
    }
    return hash->ops->insert(hash->table, key, key_size, value, value_size);
}

// Look up a key in the generic hash table
void* hash_lookup(generic_hash_t* hash, const void* key, size_t key_size,
                 size_t* value_size_ptr) {
    if (!hash || !hash->ops) {
        return NULL;
    }
    return hash->ops->lookup(hash->table, key, key_size, value_size_ptr);
}

// Delete a key-value pair from the generic hash table
bool hash_delete(generic_hash_t* hash, const void* key, size_t key_size) {
    if (!hash || !hash->ops) {
        return false;
    }
    return hash->ops->delete(hash->table, key, key_size);
}

// Destroy the generic hash table and free all memory
void hash_destroy(generic_hash_t* hash) {
    if (!hash || !hash->ops) {
        return;
    }
    hash->ops->destroy(hash->table);
    free(hash);
}

// Get statistics from the generic hash table
hash_stats_t* hash_get_stats(generic_hash_t* hash) {
    if (!hash || !hash->ops) {
        return NULL;
    }
    return hash->ops->get_stats(hash->table);
}

// Get the current load factor of the hash table
double hash_get_load_factor(generic_hash_t* hash) {
    if (!hash || !hash->ops || !hash->table) {
        return 0.0;
    }

    // Use the statistics structure instead of direct struct access
    hash_stats_t* stats = hash_get_stats(hash);
    if (!stats) {
        return 0.0;
    }

    // Calculate load factor from the stats, avoiding division by zero
    if (stats->table_size == 0) {
        return 0.0;
    }

    return (double)stats->num_entries / stats->table_size;
}

// 获取插入操作的平均探测次数
double hash_get_avg_insert_probes(generic_hash_t* hash) {
    if (!hash || !hash->ops || !hash->table) {
        return 0.0;
    }

    hash_stats_t* stats = hash->ops->get_stats(hash->table);
    if (!stats || stats->insert_ops == 0) {
        return 0.0;
    }

    return (double)stats->insert_probes / stats->insert_ops;
}

// 获取查找操作的平均探测次数
double hash_get_avg_lookup_probes(generic_hash_t* hash) {
    if (!hash || !hash->ops || !hash->table) {
        return 0.0;
    }

    hash_stats_t* stats = hash->ops->get_stats(hash->table);
    if (!stats || stats->lookup_ops == 0) {
        return 0.0;
    }

    return (double)stats->lookup_probes / stats->lookup_ops;
}

// 获取删除操作的平均探测次数
double hash_get_avg_delete_probes(generic_hash_t* hash) {
    if (!hash || !hash->ops || !hash->table) {
        return 0.0;
    }

    hash_stats_t* stats = hash->ops->get_stats(hash->table);
    if (!stats || stats->delete_ops == 0) {
        return 0.0;
    }

    return (double)stats->delete_probes / stats->delete_ops;
}