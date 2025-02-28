#include "../include/uniform_hash.h"
#include <stdlib.h>
#include <string.h>

// 均匀哈希表结构（使用双重哈希）
typedef struct uniform_hash {
    hash_entry_t* slots;
    size_t size;
    size_t num_elements;
    double max_load_factor;
    hash_function hash_func;
    hash_stats_t stats;
} uniform_hash_t;

// 创建一个新的均匀哈希表（使用双重哈希）
uniform_hash_t* uniform_hash_create(size_t size, double max_load_factor, hash_function hash_func) {
    if (size == 0 || max_load_factor <= 0.0 || max_load_factor >= 1.0) {
        return NULL;
    }

    uniform_hash_t* table = (uniform_hash_t*)safe_malloc(sizeof(uniform_hash_t));

    table->size = size;
    table->slots = (hash_entry_t*)safe_calloc(size, sizeof(hash_entry_t));
    table->num_elements = 0;
    table->max_load_factor = max_load_factor;
    table->hash_func = hash_func ? hash_func : default_hash;

    init_hash_stats(&table->stats);
    table->stats.table_size = table->size;

    return table;
}

// 插入键值对
bool uniform_hash_insert(uniform_hash_t* table, const void* key, size_t key_size,
                        const void* value, size_t value_size) {
    if (!table || !key || !value) {
        return false;
    }

    // 检查负载因子是否已经达到最大值
    if ((double)(table->num_elements + 1) / table->size > table->max_load_factor) {
        return false;
    }

    // Initialize probe count to 1, since the first access is already a probe
    uint32_t probe_count = 1;

    // 双重哈希探测
    for (uint32_t i = 0; i < table->size; i++) {
        // 使用双重哈希函数计算位置
        uint32_t pos = table->hash_func(key, key_size, i) % table->size;
        probe_count++;

        // 检查槽是否已占用
        if (!table->slots[pos].occupied) {
            // 找到空槽，插入新键值对
            table->slots[pos].key = safe_malloc(key_size);
            table->slots[pos].key_size = key_size;
            table->slots[pos].value = safe_malloc(value_size);
            table->slots[pos].value_size = value_size;

            memcpy(table->slots[pos].key, key, key_size);
            memcpy(table->slots[pos].value, value, value_size);
            table->slots[pos].occupied = true;

            table->num_elements++;
            table->stats.num_entries = table->num_elements;

            // Update statistics
            table->stats.insert_ops++;
            table->stats.insert_probes += probe_count;
            update_hash_stats(&table->stats, probe_count);

            return true;
        } else if (table->slots[pos].key_size == key_size &&
                  memcmp(table->slots[pos].key, key, key_size) == 0) {
            // 键已经存在，替换值
            free(table->slots[pos].value);

            // 分配并复制新值
            table->slots[pos].value = safe_malloc(value_size);
            table->slots[pos].value_size = value_size;
            memcpy(table->slots[pos].value, value, value_size);

            // Update statistics
            table->stats.insert_ops++;
            table->stats.insert_probes += probe_count;
            update_hash_stats(&table->stats, probe_count);

            return true;
        }

        // 如果槽已占用但键不匹配，继续探测
    }

    // 如果我们检查了整个表仍未找到位置，则返回失败
    return false;
}

// 通过键查找值
void* uniform_hash_lookup(uniform_hash_t* table, const void* key, size_t key_size, size_t* value_size) {
    if (!table || !key) {
        return NULL;
    }

    // Initialize probe count to 1, since the first access is already a probe
    uint32_t probe_count = 1;

    // 双重哈希探测
    for (uint32_t i = 0; i < table->size; i++) {
        // 使用双重哈希函数计算位置
        uint32_t pos = table->hash_func(key, key_size, i) % table->size;
        probe_count++;

        // 检查槽是否已占用
        if (!table->slots[pos].occupied) {
            // 到达空槽，键不在表中
            break;
        }

        // 检查键是否匹配
        if (table->slots[pos].key_size == key_size &&
            memcmp(table->slots[pos].key, key, key_size) == 0) {

            // 找到键
            if (value_size) {
                *value_size = table->slots[pos].value_size;
            }

            // Update statistics
            table->stats.lookup_ops++;
            table->stats.lookup_probes += probe_count;
            update_hash_stats(&table->stats, probe_count);

            return table->slots[pos].value;
        }

        // 如果槽已占用但键不匹配，继续探测
    }

    // Update statistics for unsuccessful lookup
    table->stats.lookup_ops++;
    table->stats.lookup_probes += probe_count;
    update_hash_stats(&table->stats, probe_count);

    // 未找到键
    return NULL;
}

// 删除键值对
bool uniform_hash_delete(uniform_hash_t* table, const void* key, size_t key_size) {
    if (!table || !key) {
        return false;
    }

    // Initialize probe count to 1, since the first access is already a probe
    uint32_t probe_count = 1;

    // 双重哈希探测
    for (uint32_t i = 0; i < table->size; i++) {
        // 使用双重哈希函数计算位置
        uint32_t pos = table->hash_func(key, key_size, i) % table->size;
        probe_count++;

        // 检查槽是否已占用
        if (!table->slots[pos].occupied) {
            // 到达空槽，键不在表中
            break;
        }

        // 检查键是否匹配
        if (table->slots[pos].key_size == key_size &&
            memcmp(table->slots[pos].key, key, key_size) == 0) {

            // 找到键，释放内存
            free(table->slots[pos].key);
            free(table->slots[pos].value);

            // 标记为未占用
            table->slots[pos].occupied = false;
            table->num_elements--;
            table->stats.num_entries = table->num_elements;

            // Update statistics
            table->stats.delete_ops++;
            table->stats.delete_probes += probe_count;
            update_hash_stats(&table->stats, probe_count);

            return true;
        }

        // 如果槽已占用但键不匹配，继续探测
    }

    // Update statistics for unsuccessful delete
    table->stats.delete_ops++;
    table->stats.delete_probes += probe_count;
    update_hash_stats(&table->stats, probe_count);

    // 未找到键
    return false;
}

// 销毁哈希表并释放内存
void uniform_hash_destroy(uniform_hash_t* table) {
    if (!table) {
        return;
    }

    // 释放所有占用的槽
    for (size_t i = 0; i < table->size; i++) {
        if (table->slots[i].occupied) {
            free(table->slots[i].key);
            free(table->slots[i].value);
        }
    }

    // 释放槽数组和表结构
    free(table->slots);
    free(table);
}

// 获取哈希表统计信息
static hash_stats_t* uniform_hash_get_stats(void* table) {
    if (!table) return NULL;
    return &((uniform_hash_t*)table)->stats;
}

const hash_ops_t* uniform_hash_get_ops(void) {
    static const hash_ops_t ops = {
        .create = (void* (*)(size_t, double, hash_function))uniform_hash_create,
        .insert = (bool (*)(void*, const void*, size_t, const void*, size_t))uniform_hash_insert,
        .lookup = (void* (*)(void*, const void*, size_t, size_t*))uniform_hash_lookup,
        .delete = (bool (*)(void*, const void*, size_t))uniform_hash_delete,
        .destroy = (void (*)(void*))uniform_hash_destroy,
        .get_stats = uniform_hash_get_stats
    };
    return &ops;
}