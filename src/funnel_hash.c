#include "../include/funnel_hash.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Constants
#define MAX_LEVELS 8
#define MIN_LEVEL_SIZE 4

// Function declarations (forward declarations)
static double calculate_threshold(int level);

// 漏斗哈希表级别结构
typedef struct funnel_level {
    hash_entry_t* slots;
    size_t size;
    size_t occupied;
    double threshold;
} funnel_level_t;

// 主漏斗哈希表结构
typedef struct funnel_hash {
    funnel_level_t* levels;
    size_t num_levels;
    size_t total_size;
    size_t total_elements;
    double delta;
    hash_function hash_func;
    hash_stats_t stats;
} funnel_hash_t;

// Utility function to calculate threshold for a level
static double calculate_threshold(int level) {
    // As per the paper, thresholds increase with level
    return 1.0 - 1.0/(1 << (2*level));
}

// Level-specific hash function
static uint32_t level_hash(const void* key, size_t key_size, uint32_t level, uint32_t attempt) {
    // Use different hash functions for different levels to minimize conflicts
    uint32_t hash_val = default_hash(key, key_size, attempt);

    // Apply level-specific transformation
    switch (level) {
        case 0: return hash_val;
        case 1: return hash_val ^ (hash_val >> 16);
        case 2: return hash_val ^ (hash_val >> 8) ^ (hash_val >> 16) ^ (hash_val >> 24);
        default: return hash_val ^ (level * 0x9e3779b9);  // Use a prime factor
    }
}

// Create a new funnel hash table
static funnel_hash_t* funnel_hash_create(size_t size, double delta, hash_function hash_func) {
    if (size == 0 || delta <= 0 || delta >= 1) {
        return NULL;
    }

    funnel_hash_t* table = (funnel_hash_t*)safe_malloc(sizeof(funnel_hash_t));

    // Calculate the number of levels based on log(delta^-1)
    // As per the paper, we need log(delta^-1)/4 levels
    table->num_levels = (size_t)ceil(log2(1.0/delta) / 4) + 1;
    table->levels = (funnel_level_t*)safe_calloc(table->num_levels, sizeof(funnel_level_t));

    // Initialize levels with decreasing sizes
    size_t total_allocated = 0;
    for (size_t i = 0; i < table->num_levels && i < MAX_LEVELS; i++) {
        size_t level_size;

        if (i == 0) {
            level_size = size / 2;
        } else if (i == table->num_levels - 1) {
            level_size = size - total_allocated;
        } else {
            level_size = size / (1 << (i+1));
        }

        if (level_size < MIN_LEVEL_SIZE) {
            level_size = MIN_LEVEL_SIZE; // Ensure minimum size
        }

        table->levels[i].size = level_size;
        table->levels[i].slots = (hash_entry_t*)safe_calloc(level_size, sizeof(hash_entry_t));
        table->levels[i].occupied = 0;
        table->levels[i].threshold = calculate_threshold(i); // Use threshold instead of occupancy_threshold

        total_allocated += level_size;
    }

    table->total_size = size;
    table->total_elements = 0;
    table->delta = delta;
    table->hash_func = hash_func ? hash_func : default_hash;

    init_hash_stats(&table->stats);
    table->stats.table_size = table->total_size;

    return table;
}

// Insert a key-value pair into the funnel hash table
static bool funnel_hash_insert(funnel_hash_t* table, const void* key, size_t key_size,
                        const void* value, size_t value_size, uint32_t* probe_count_ptr) {
    if (!table || !key || !value) {
        return false;
    }

    // Check if we have enough space overall
    if (table->total_elements >= table->total_size * (1.0 - table->delta)) {
        return false; // Table too full
    }

    uint32_t probe_count = 0;

    // Try to insert into each level according to the funnel hashing algorithm
    for (size_t level = 0; level < table->num_levels; level++) {
        funnel_level_t* current_level = &table->levels[level];

        // Check if this level is too full based on its threshold
        double load_factor = (double)current_level->occupied / current_level->size;
        if (load_factor >= current_level->threshold) {
            continue; // This level is too full, try the next one
        }

        // Try to find an empty slot in this level
        for (uint32_t attempt = 0; attempt < current_level->size; attempt++) {
            uint32_t hash_val = level_hash(key, key_size, level, attempt);
            uint32_t pos = hash_val % current_level->size;
            probe_count++;

            if (!current_level->slots[pos].occupied) {
                // Found an empty slot, insert here
                current_level->slots[pos].key = malloc(key_size);
                current_level->slots[pos].key_size = key_size;
                current_level->slots[pos].value = malloc(value_size);
                current_level->slots[pos].value_size = value_size;

                memcpy(current_level->slots[pos].key, key, key_size);
                memcpy(current_level->slots[pos].value, value, value_size);
                current_level->slots[pos].occupied = true;

                current_level->occupied++;
                table->total_elements++;

                // Store probe count if pointer is provided
                if (probe_count_ptr) {
                    *probe_count_ptr = probe_count;
                }

                update_hash_stats(&table->stats, probe_count);
                table->stats.insert_probes += probe_count;
                table->stats.insert_ops++;
                return true;
            }
        }
    }

    // If we get here, insertion failed despite space being available
    return false;
}

// Lookup a key in the funnel hash table
static void* funnel_hash_lookup(funnel_hash_t* table, const void* key, size_t key_size,
                              size_t* value_size_ptr, uint32_t* probe_count_ptr) {
    if (!table || !key) {
        return NULL;
    }

    uint32_t probe_count = 0;

    // Check each level for the key
    for (size_t level = 0; level < table->num_levels; level++) {
        funnel_level_t* current_level = &table->levels[level];

        // Check each position in the hash sequence for this level
        for (uint32_t attempt = 0; attempt < current_level->size; attempt++) {
            uint32_t hash_val = level_hash(key, key_size, level, attempt);
            uint32_t pos = hash_val % current_level->size;
            probe_count++;

            // Check if this slot is occupied
            if (current_level->slots[pos].occupied) {
                // Check if the key matches
                if (current_level->slots[pos].key_size == key_size &&
                    memcmp(current_level->slots[pos].key, key, key_size) == 0) {

                    if (value_size_ptr) {
                        *value_size_ptr = current_level->slots[pos].value_size;
                    }

                    // Store probe count if pointer is provided
                    if (probe_count_ptr) {
                        *probe_count_ptr = probe_count;
                    }

                    update_hash_stats(&table->stats, probe_count);
                    table->stats.lookup_probes += probe_count;
                    table->stats.lookup_ops++;
                    return current_level->slots[pos].value;
                }
            } else {
                // We've reached an empty slot, so the key cannot be in this sequence
                break;
            }
        }
    }

    // Key not found
    update_hash_stats(&table->stats, probe_count);
    table->stats.lookup_probes += probe_count;
    table->stats.lookup_ops++;
    return NULL;
}

// Delete a key from the funnel hash table
static bool funnel_hash_delete(funnel_hash_t* table, const void* key, size_t key_size,
                             uint32_t* probe_count_ptr) {
    if (!table || !key) {
        return false;
    }

    uint32_t probe_count = 0;

    // Search for the key in all levels
    for (size_t level = 0; level < table->num_levels; level++) {
        funnel_level_t* current_level = &table->levels[level];

        // Check each position in the hash sequence for this level
        for (uint32_t attempt = 0; attempt < current_level->size; attempt++) {
            uint32_t hash_val = level_hash(key, key_size, level, attempt);
            uint32_t pos = hash_val % current_level->size;
            probe_count++;

            // Check if this slot contains our key
            if (current_level->slots[pos].occupied &&
                current_level->slots[pos].key_size == key_size &&
                memcmp(current_level->slots[pos].key, key, key_size) == 0) {

                // Free memory
                free(current_level->slots[pos].key);
                free(current_level->slots[pos].value);

                // Mark as unoccupied
                current_level->slots[pos].occupied = false;
                current_level->occupied--;
                table->total_elements--;

                // Store probe count if pointer is provided
                if (probe_count_ptr) {
                    *probe_count_ptr = probe_count;
                }

                update_hash_stats(&table->stats, probe_count);
                table->stats.delete_probes += probe_count;
                table->stats.delete_ops++;
                return true;
            } else if (!current_level->slots[pos].occupied) {
                // Empty slot, so key isn't here
                break;
            }
        }
    }

    // Key not found
    update_hash_stats(&table->stats, probe_count);
    table->stats.delete_probes += probe_count;
    table->stats.delete_ops++;
    return false;
}

// Destroy the funnel hash table and free memory
static void funnel_hash_destroy(funnel_hash_t* table) {
    if (!table) {
        return;
    }

    // Free all levels and their contents
    for (size_t i = 0; i < table->num_levels; i++) {
        funnel_level_t* level = &table->levels[i];

        // Free all occupied slots
        for (size_t j = 0; j < level->size; j++) {
            if (level->slots[j].occupied) {
                if (level->slots[j].key) {
                    free(level->slots[j].key);
                }
                if (level->slots[j].value) {
                    free(level->slots[j].value);
                }
            }
        }

        // Free the slots array
        if (level->slots) {
            free(level->slots);
        }
    }

    // Free the levels array and the table itself
    if (table->levels) {
        free(table->levels);
    }
    free(table);
}

// Get hash table statistics
static hash_stats_t* funnel_hash_get_stats(void* table) {
    if (!table) return NULL;
    return &((funnel_hash_t*)table)->stats;
}

// Wrapper functions with simplified signatures for the generic interface

// Wrapper for insert
static bool funnel_hash_insert_wrapper(funnel_hash_t* table, const void* key, size_t key_size,
                              const void* value, size_t value_size) {
    return funnel_hash_insert(table, key, key_size, value, value_size, NULL);
}

// Wrapper for lookup
static void* funnel_hash_lookup_wrapper(funnel_hash_t* table, const void* key, size_t key_size,
                                      size_t* value_size_ptr) {
    return funnel_hash_lookup(table, key, key_size, value_size_ptr, NULL);
}

// Wrapper for delete
static bool funnel_hash_delete_wrapper(funnel_hash_t* table, const void* key, size_t key_size) {
    return funnel_hash_delete(table, key, key_size, NULL);
}

// Return the operations structure for funnel hash tables
const hash_ops_t* funnel_hash_get_ops(void) {
    static const hash_ops_t ops = {
        .create = (void* (*)(size_t, double, hash_function))funnel_hash_create,
        .insert = (bool (*)(void*, const void*, size_t, const void*, size_t))funnel_hash_insert_wrapper,
        .lookup = (void* (*)(void*, const void*, size_t, size_t*))funnel_hash_lookup_wrapper,
        .delete = (bool (*)(void*, const void*, size_t))funnel_hash_delete_wrapper,
        .destroy = (void (*)(void*))funnel_hash_destroy,
        .get_stats = funnel_hash_get_stats
    };
    return &ops;
}