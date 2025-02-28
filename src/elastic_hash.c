#include "../include/elastic_hash.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Structure to define a sub-array in elastic hashing
typedef struct {
    hash_entry_t* slots;
    size_t size;
    size_t occupied;
} elastic_subarray_t;

// Main elastic hash table structure
typedef struct {
    elastic_subarray_t* subarrays;
    size_t num_subarrays;
    size_t total_size;
    size_t total_elements;
    double delta;
    hash_function hash_func;
    hash_stats_t stats;
} elastic_hash_t;

// Forward declarations
static uint32_t calculate_f(double epsilon, double delta);
static uint32_t optimal_probe_mapping(uint32_t hash_val, uint32_t attempt, uint32_t subarray_size);
static bool adjust_subarrays_if_needed(elastic_hash_t* table);

// Implementation of optimal probe mapping
static uint32_t optimal_probe_mapping(uint32_t hash_val, uint32_t attempt, uint32_t subarray_size) {
    // Implementation based on the paper's specification
    // This function maps hash values to positions to minimize clustering
    // while maintaining O(1) probe count bounds

    if (subarray_size <= 1) return 0;

    // Double hashing style approach for optimal distribution
    return (hash_val + attempt * (1 + (hash_val % (subarray_size - 1)))) % subarray_size;
}

// Implementation of dynamic resizing check
static bool adjust_subarrays_if_needed(elastic_hash_t* table) {
    // Implementation to dynamically resize subarrays based on load factors
    // Uses existing thresholds in the elastic_hash_t structure

    // Calculate current load factor
    double current_load = (double)table->total_elements / table->total_size;
    double target_threshold = 1.0 - table->delta;

    // Check if we need to expand
    if (current_load > target_threshold * 0.9) {
        // This would be where we'd add a new subarray, but we'll
        // just return false for now since expansion is handled elsewhere
        return false;
    }

    return true;
}

// Implementation of the phi mapping function as described in the paper
uint32_t phi_mapping(uint32_t i, uint32_t j) {
    // Based on the injection φ:Z^+ × Z^+ → Z^+ in the paper
    // This ensures phi(i,j) <= O(i * j^2)
    uint32_t result = 1;

    // Encode j's bits interleaved with 1s
    uint32_t temp_j = j;
    while (temp_j > 0) {
        result = (result << 2) | ((temp_j & 1) << 1) | 1;
        temp_j >>= 1;
    }

    // Append a 0 and then i's bits
    result = (result << 1);
    uint32_t temp_i = i;
    while (temp_i > 0) {
        result = (result << 1) | (temp_i & 1);
        temp_i >>= 1;
    }

    return result;
}

// Function to calculate probe position in a specific subarray
uint32_t elastic_get_probe_pos(elastic_hash_t* table, const void* key, size_t key_size, uint32_t i, uint32_t j) {
    if (i >= table->num_subarrays) {
        return 0; // Error case
    }

    elastic_subarray_t* subarray = &table->subarrays[i];
    uint32_t hash_val = table->hash_func(key, key_size, phi_mapping(i+1, j+1));

    // Use our optimal probe mapping for better distribution
    return optimal_probe_mapping(hash_val, j, subarray->size);
}

// Helper function to calculate F value in the paper's algorithm
static uint32_t calculate_f(double epsilon, double delta) {
    if (epsilon <= 0 || delta <= 0) return 1;
    return (uint32_t)ceil(4.0 * log(2.0 / delta) / epsilon);
}

// Create a new elastic hash table
elastic_hash_t* elastic_hash_create(size_t size, double delta, hash_function hash_func) {
    if (size == 0 || delta <= 0 || delta >= 1) {
        return NULL;
    }

    elastic_hash_t* table = (elastic_hash_t*)safe_malloc(sizeof(elastic_hash_t));

    // Calculate number of subarrays based on log(n)
    table->num_subarrays = (size_t)ceil(log2(size)) + 1;
    table->subarrays = (elastic_subarray_t*)safe_calloc(table->num_subarrays, sizeof(elastic_subarray_t));

    // Initialize subarrays with decreasing sizes
    size_t total_allocated = 0;

    for (size_t i = 0; i < table->num_subarrays; i++) {
        size_t subarray_size;
        if (i == 0) {
            subarray_size = size / 2;
        } else if (i == table->num_subarrays - 1) {
            subarray_size = size - total_allocated;
        } else {
            subarray_size = size / (2 << i);
        }

        if (subarray_size == 0) {
            subarray_size = 1; // Ensure at least one slot
        }

        table->subarrays[i].size = subarray_size;
        table->subarrays[i].slots = (hash_entry_t*)safe_calloc(subarray_size, sizeof(hash_entry_t));
        table->subarrays[i].occupied = 0;

        total_allocated += subarray_size;
    }

    table->total_size = size;
    table->total_elements = 0;
    table->delta = delta;
    table->hash_func = hash_func ? hash_func : default_hash;

    init_hash_stats(&table->stats);
    table->stats.table_size = table->total_size;

    return table;
}

// Insert a key-value pair into the elastic hash table
bool elastic_hash_insert(elastic_hash_t* table, const void* key, size_t key_size,
                         const void* value, size_t value_size, uint32_t* probe_count_ptr) {
    if (!table || !key || !value) {
        return false;
    }

    // Check if we have enough space overall
    if (table->total_elements >= table->total_size * (1.0 - table->delta)) {
        return false; // Table too full
    }

    // Determine which batch we're in
    size_t batch_index = 0;
    while (batch_index < table->num_subarrays - 1) {
        elastic_subarray_t* current = &table->subarrays[batch_index];
        size_t target_fill = current->size - (size_t)(table->delta * current->size / 2);

        if (current->occupied < target_fill) {
            break;
        }

        batch_index++;
    }

    // Track probe count for statistics
    uint32_t probe_count = 0;

    // Handle insertion based on which array(s) to consider
    if (batch_index == 0) {
        // Special case for first batch: just fill array A₁ to 75%
        elastic_subarray_t* first_array = &table->subarrays[0];

        // Try each probe position in sequence
        for (uint32_t j = 0; j < first_array->size; j++) {
            uint32_t pos = elastic_get_probe_pos(table, key, key_size, 0, j);
            probe_count++;

            if (!first_array->slots[pos].occupied) {
                // Found an empty slot, insert here
                first_array->slots[pos].key = malloc(key_size);
                first_array->slots[pos].key_size = key_size;
                first_array->slots[pos].value = malloc(value_size);
                first_array->slots[pos].value_size = value_size;

                memcpy(first_array->slots[pos].key, key, key_size);
                memcpy(first_array->slots[pos].value, value, value_size);
                first_array->slots[pos].occupied = true;

                first_array->occupied++;
                table->total_elements++;

                // Update statistics
                table->stats.num_entries = table->total_elements;
                table->stats.insert_ops++;
                table->stats.insert_probes += probe_count;
                update_hash_stats(&table->stats, probe_count);

                if (probe_count_ptr) {
                    *probe_count_ptr = probe_count;
                }

                // Check if table needs adjustment after insertion
                adjust_subarrays_if_needed(table);

                return true;
            }
        }

        // If we reach here, insertion failed despite space being available
        return false;
    } else {
        // For other batches, implementation based on the paper's algorithm
        elastic_subarray_t* array_i = &table->subarrays[batch_index];
        elastic_subarray_t* array_i_plus_1 =
            (batch_index < table->num_subarrays - 1) ?
            &table->subarrays[batch_index + 1] : NULL;

        // Calculate epsilon values
        double epsilon_1 = 1.0 - (double)array_i->occupied / array_i->size;
        double epsilon_2 = array_i_plus_1 ?
                          1.0 - (double)array_i_plus_1->occupied / array_i_plus_1->size :
                          0.0;

        // Case 1: Can go in either array (based on paper's algorithm)
        if (epsilon_1 > table->delta/2 && epsilon_2 > 0.25) {
            // Try array_i first with limited probes
            uint32_t f_value = calculate_f(epsilon_1, table->delta);

            for (uint32_t j = 0; j < f_value; j++) {
                uint32_t pos = elastic_get_probe_pos(table, key, key_size, batch_index, j);
                probe_count++;

                if (!array_i->slots[pos].occupied) {
                    // Found empty slot in array_i
                    array_i->slots[pos].key = malloc(key_size);
                    array_i->slots[pos].key_size = key_size;
                    array_i->slots[pos].value = malloc(value_size);
                    array_i->slots[pos].value_size = value_size;

                    memcpy(array_i->slots[pos].key, key, key_size);
                    memcpy(array_i->slots[pos].value, value, value_size);
                    array_i->slots[pos].occupied = true;

                    array_i->occupied++;
                    table->total_elements++;

                    // Update statistics
                    table->stats.num_entries = table->total_elements;
                    table->stats.insert_ops++;
                    table->stats.insert_probes += probe_count;
                    update_hash_stats(&table->stats, probe_count);

                    if (probe_count_ptr) {
                        *probe_count_ptr = probe_count;
                    }

                    // Check if table needs adjustment after insertion
                    adjust_subarrays_if_needed(table);

                    return true;
                }
            }

            // If not found in array_i, try array_i_plus_1
            if (array_i_plus_1) {
                for (uint32_t j = 0; j < array_i_plus_1->size; j++) {
                    uint32_t pos = elastic_get_probe_pos(table, key, key_size, batch_index + 1, j);
                    probe_count++;

                    if (!array_i_plus_1->slots[pos].occupied) {
                        // Found empty slot in array_i_plus_1
                        array_i_plus_1->slots[pos].key = malloc(key_size);
                        array_i_plus_1->slots[pos].key_size = key_size;
                        array_i_plus_1->slots[pos].value = malloc(value_size);
                        array_i_plus_1->slots[pos].value_size = value_size;

                        memcpy(array_i_plus_1->slots[pos].key, key, key_size);
                        memcpy(array_i_plus_1->slots[pos].value, value, value_size);
                        array_i_plus_1->slots[pos].occupied = true;

                        array_i_plus_1->occupied++;
                        table->total_elements++;

                        // Update statistics
                        table->stats.num_entries = table->total_elements;
                        table->stats.insert_ops++;
                        table->stats.insert_probes += probe_count;
                        update_hash_stats(&table->stats, probe_count);

                        if (probe_count_ptr) {
                            *probe_count_ptr = probe_count;
                        }

                        // Check if table needs adjustment after insertion
                        adjust_subarrays_if_needed(table);

                        return true;
                    }
                }
            }
        }
        // Case 2: Must go to array_i_plus_1
        else if (epsilon_1 <= table->delta/2 && array_i_plus_1) {
            for (uint32_t j = 0; j < array_i_plus_1->size; j++) {
                uint32_t pos = elastic_get_probe_pos(table, key, key_size, batch_index + 1, j);
                probe_count++;

                if (!array_i_plus_1->slots[pos].occupied) {
                    // Found empty slot
                    array_i_plus_1->slots[pos].key = malloc(key_size);
                    array_i_plus_1->slots[pos].key_size = key_size;
                    array_i_plus_1->slots[pos].value = malloc(value_size);
                    array_i_plus_1->slots[pos].value_size = value_size;

                    memcpy(array_i_plus_1->slots[pos].key, key, key_size);
                    memcpy(array_i_plus_1->slots[pos].value, value, value_size);
                    array_i_plus_1->slots[pos].occupied = true;

                    array_i_plus_1->occupied++;
                    table->total_elements++;

                    // Update statistics
                    table->stats.num_entries = table->total_elements;
                    table->stats.insert_ops++;
                    table->stats.insert_probes += probe_count;
                    update_hash_stats(&table->stats, probe_count);

                    if (probe_count_ptr) {
                        *probe_count_ptr = probe_count;
                    }

                    // Check if table needs adjustment after insertion
                    adjust_subarrays_if_needed(table);

                    return true;
                }
            }
        }
        // Case 3: Must go to array_i (expensive case)
        else if (epsilon_2 <= 0.25) {
            for (uint32_t j = 0; j < array_i->size; j++) {
                uint32_t pos = elastic_get_probe_pos(table, key, key_size, batch_index, j);
                probe_count++;

                if (!array_i->slots[pos].occupied) {
                    // Found empty slot
                    array_i->slots[pos].key = malloc(key_size);
                    array_i->slots[pos].key_size = key_size;
                    array_i->slots[pos].value = malloc(value_size);
                    array_i->slots[pos].value_size = value_size;

                    memcpy(array_i->slots[pos].key, key, key_size);
                    memcpy(array_i->slots[pos].value, value, value_size);
                    array_i->slots[pos].occupied = true;

                    array_i->occupied++;
                    table->total_elements++;

                    // Update statistics
                    table->stats.num_entries = table->total_elements;
                    table->stats.insert_ops++;
                    table->stats.insert_probes += probe_count;
                    update_hash_stats(&table->stats, probe_count);

                    if (probe_count_ptr) {
                        *probe_count_ptr = probe_count;
                    }

                    // Check if table needs adjustment after insertion
                    adjust_subarrays_if_needed(table);

                    return true;
                }
            }
        }
    }

    // If we get here, insertion failed
    return false;
}

// Lookup a key in the elastic hash table
void* elastic_hash_lookup(elastic_hash_t* table, const void* key, size_t key_size, size_t* value_size_ptr) {
    if (!table || !key) return NULL;

    uint32_t probe_count = 0;
    const uint32_t max_total_probes = 200; // Absolute maximum for entire search

    // We need to check positions across subarrays, but with stricter limits
    for (size_t i = 0; i < table->num_subarrays && probe_count < max_total_probes; i++) {
        elastic_subarray_t* subarray = &table->subarrays[i];

        // Limit probes per subarray even more aggressively
        uint32_t max_probes = MIN(20, (uint32_t)log2(subarray->size + 1) * 3);

        // For each position in the hash function sequence
        for (uint32_t j = 0; j < max_probes && probe_count < max_total_probes; j++) {
            uint32_t pos = elastic_get_probe_pos(table, key, key_size, i, j);
            probe_count++;

            // Check if this slot is occupied
            if (subarray->slots[pos].occupied) {
                // Check if the key matches
                if (subarray->slots[pos].key_size == key_size &&
                    memcmp(subarray->slots[pos].key, key, key_size) == 0) {

                    if (value_size_ptr) {
                        *value_size_ptr = subarray->slots[pos].value_size;
                    }

                    // Update statistics
                    table->stats.lookup_ops++;
                    table->stats.lookup_probes += probe_count;
                    update_hash_stats(&table->stats, probe_count);

                    return subarray->slots[pos].value;
                }
            } else {
                // We've reached an empty slot, so the key cannot be in this sequence
                break;
            }
        }
    }

    // Update statistics even for unsuccessful lookups
    table->stats.lookup_ops++;
    table->stats.lookup_probes += probe_count;
    update_hash_stats(&table->stats, probe_count);

    // Key not found
    return NULL;
}

// Delete a key from the elastic hash table
bool elastic_hash_delete(elastic_hash_t* table, const void* key, size_t key_size) {
    if (!table || !key) {
        return false;
    }

    // Track probe count for statistics
    uint32_t probe_count = 0;

    // Search for the key in all subarrays
    for (size_t i = 0; i < table->num_subarrays; i++) {
        elastic_subarray_t* subarray = &table->subarrays[i];

        // Check each position in the hash function sequence
        for (uint32_t j = 0; j < subarray->size; j++) {
            uint32_t pos = elastic_get_probe_pos(table, key, key_size, i, j);
            probe_count++;  // Increment probe count for each probe

            // Check if this slot contains our key
            if (subarray->slots[pos].occupied &&
                subarray->slots[pos].key_size == key_size &&
                memcmp(subarray->slots[pos].key, key, key_size) == 0) {

                // Free memory
                free(subarray->slots[pos].key);
                free(subarray->slots[pos].value);

                // Mark as unoccupied
                subarray->slots[pos].occupied = false;
                subarray->occupied--;
                table->total_elements--;

                // Update statistics
                table->stats.delete_ops++;
                table->stats.delete_probes += probe_count;
                table->stats.num_entries = table->total_elements;
                update_hash_stats(&table->stats, probe_count);

                return true;
            } else if (!subarray->slots[pos].occupied) {
                // Empty slot, so key isn't here
                break;
            }
        }
    }

    // Update statistics even for unsuccessful deletes
    table->stats.delete_ops++;
    table->stats.delete_probes += probe_count;
    update_hash_stats(&table->stats, probe_count);

    // Key not found
    return false;
}

// Destroy the elastic hash table and free memory
void elastic_hash_destroy(elastic_hash_t* table) {
    if (!table) {
        return;
    }

    // Free all subarrays and their contents
    for (size_t i = 0; i < table->num_subarrays; i++) {
        elastic_subarray_t* subarray = &table->subarrays[i];

        // Free all occupied slots
        for (size_t j = 0; j < subarray->size; j++) {
            if (subarray->slots[j].occupied) {
                free(subarray->slots[j].key);
                free(subarray->slots[j].value);
            }
        }

        // Free the slots array
        free(subarray->slots);
    }

    // Free the subarrays array and the table itself
    free(table->subarrays);
    free(table);
}

// Wrapper for insert
static bool elastic_hash_insert_wrapper(void* table_ptr, const void* key, size_t key_size,
                                      const void* value, size_t value_size) {
    elastic_hash_t* table = (elastic_hash_t*)table_ptr;
    uint32_t probe_count = 0;
    bool result = elastic_hash_insert(table, key, key_size, value, value_size, &probe_count);

    // Check if adjustment is needed after insertion
    if (result) {
        adjust_subarrays_if_needed(table);
    }

    return result;
}

// Get hash table statistics
static hash_stats_t* elastic_hash_get_stats(void* table) {
    if (!table) return NULL;
    return &((elastic_hash_t*)table)->stats;
}

const hash_ops_t* elastic_hash_get_ops(void) {
    static const hash_ops_t ops = {
        .create = (void* (*)(size_t, double, hash_function))elastic_hash_create,
        .insert = (bool (*)(void*, const void*, size_t, const void*, size_t))elastic_hash_insert_wrapper,
        .lookup = (void* (*)(void*, const void*, size_t, size_t*))elastic_hash_lookup,
        .delete = (bool (*)(void*, const void*, size_t))elastic_hash_delete,
        .destroy = (void (*)(void*))elastic_hash_destroy,
        .get_stats = elastic_hash_get_stats
    };
    return &ops;
}