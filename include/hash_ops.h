/**
 * @file hash_ops.h
 * @brief Unified interface for hash table operations
 */

#ifndef HASH_OPS_H
#define HASH_OPS_H

#include <stddef.h>
#include <stdbool.h>
#include "common.h"

/**
 * @brief Hash table type enumeration
 */
typedef enum {
    HASH_TYPE_ELASTIC,  // From the paper
    HASH_TYPE_FUNNEL,   // From the paper
    HASH_TYPE_LINEAR,   // Standard implementation for comparison
    HASH_TYPE_UNIFORM   // Standard implementation for comparison
} hash_type_t;

/**
 * @brief Structure containing function pointers for hash table operations
 */
typedef struct {
    /**
     * @brief Create a new hash table
     * @param size Initial table size
     * @param param Additional parameter (delta for elastic/funnel, load_factor for linear/uniform)
     * @param hash_func Custom hash function (can be NULL)
     * @return Pointer to the created hash table
     */
    void* (*create)(size_t size, double param, hash_function hash_func);

    /**
     * @brief Insert a key-value pair into the hash table
     * @param table Hash table
     * @param key Key data
     * @param key_size Key size in bytes
     * @param value Value data
     * @param value_size Value size in bytes
     * @return true if insertion was successful, false otherwise
     */
    bool (*insert)(void* table, const void* key, size_t key_size,
                  const void* value, size_t value_size);

    /**
     * @brief Look up a key in the hash table
     * @param table Hash table
     * @param key Key data
     * @param key_size Key size in bytes
     * @param value_size_ptr Pointer to store the size of the value (can be NULL)
     * @return Pointer to the value if found, NULL otherwise
     */
    void* (*lookup)(void* table, const void* key, size_t key_size,
                   size_t* value_size_ptr);

    /**
     * @brief Delete a key-value pair from the hash table
     * @param table Hash table
     * @param key Key data
     * @param key_size Key size in bytes
     * @return true if deletion was successful, false otherwise
     */
    bool (*delete)(void* table, const void* key, size_t key_size);

    /**
     * @brief Destroy the hash table and free all memory
     * @param table Hash table
     */
    void (*destroy)(void* table);

    /**
     * @brief Get probe position for a key at a specific position
     * @param table Hash table
     * @param key Key data
     * @param key_size Key size in bytes
     * @param position Position in the probe sequence
     * @return The calculated probe position
     */
    size_t (*get_probe_pos)(void* table, const void* key, size_t key_size,
                           size_t position);

    /**
     * @brief Get statistics from the hash table
     * @param table Hash table
     * @return Pointer to hash table statistics
     */
    hash_stats_t* (*get_stats)(void* table);
} hash_ops_t;

/**
 * @brief Get the operations structure for a specific hash table type
 * @param type Hash table type
 * @return Pointer to the operations structure
 */
const hash_ops_t* get_hash_ops(hash_type_t type);

/**
 * @brief Generic hash table structure
 */
typedef struct {
    void* table;                  // Actual hash table implementation
    const hash_ops_t* ops;        // Operations for this hash table
    hash_type_t type;             // Hash table type
} generic_hash_t;

/**
 * @brief Create a generic hash table
 * @param type Hash table type
 * @param size Initial table size
 * @param param Additional parameter (delta for elastic/funnel, load_factor for linear/uniform)
 * @param hash_func Custom hash function (can be NULL)
 * @return Pointer to the created generic hash table
 */
generic_hash_t* hash_create(hash_type_t type, size_t size, double param,
                           hash_function hash_func);

/**
 * @brief Insert a key-value pair into the generic hash table
 * @param hash Generic hash table
 * @param key Key data
 * @param key_size Key size in bytes
 * @param value Value data
 * @param value_size Value size in bytes
 * @return true if insertion was successful, false otherwise
 */
bool hash_insert(generic_hash_t* hash, const void* key, size_t key_size,
                const void* value, size_t value_size);

/**
 * @brief Look up a key in the generic hash table
 * @param hash Generic hash table
 * @param key Key data
 * @param key_size Key size in bytes
 * @param value_size_ptr Pointer to store the size of the value (can be NULL)
 * @return Pointer to the value if found, NULL otherwise
 */
void* hash_lookup(generic_hash_t* hash, const void* key, size_t key_size,
                 size_t* value_size_ptr);

/**
 * @brief Delete a key-value pair from the generic hash table
 * @param hash Generic hash table
 * @param key Key data
 * @param key_size Key size in bytes
 * @return true if deletion was successful, false otherwise
 */
bool hash_delete(generic_hash_t* hash, const void* key, size_t key_size);

/**
 * @brief Destroy the generic hash table and free all memory
 * @param hash Generic hash table
 */
void hash_destroy(generic_hash_t* hash);

/**
 * @brief Get statistics from the generic hash table
 * @param hash Generic hash table
 * @return Pointer to hash table statistics
 */
hash_stats_t* hash_get_stats(generic_hash_t* hash);

/**
 * @brief Get the current load factor of the hash table
 * @param hash Generic hash table
 * @return Current load factor (ratio of entries to table size)
 */
double hash_get_load_factor(generic_hash_t* hash);

/**
 * @brief Get the average probe counts for insert operations
 * @param hash Generic hash table
 * @return Average probe counts for insert operations
 */
double hash_get_avg_insert_probes(generic_hash_t* hash);

/**
 * @brief Get the average probe counts for lookup operations
 * @param hash Generic hash table
 * @return Average probe counts for lookup operations
 */
double hash_get_avg_lookup_probes(generic_hash_t* hash);

/**
 * @brief Get the average probe counts for delete operations
 * @param hash Generic hash table
 * @return Average probe counts for delete operations
 */
double hash_get_avg_delete_probes(generic_hash_t* hash);

#endif /* HASH_OPS_H */