#include "../include/hash_ops.h"

// 在创建哈希表时使用统一接口
generic_hash_t* elastic_hash = hash_create(HASH_TYPE_ELASTIC, table_size, delta, NULL);

// 在获取统计信息时使用统一接口
hash_stats_t* elastic_stats = hash_get_stats(elastic_hash);

// 在插入/查找/删除时使用统一接口
hash_insert(elastic_hash, key, key_size, value, value_size);

// 在销毁哈希表时使用统一接口
hash_destroy(elastic_hash);

// 打印表头
printf("+------------------+--------------+--------------+--------------+--------------+------------+\n");
printf("| Hash Type        | Avg Probes   | Insert Probes| Lookup Probes| Delete Probes| Load Factor |\n");
printf("+------------------+--------------+--------------+--------------+--------------+------------+\n");

// 打印弹性哈希表结果
printf("| %-16s | %-12.2f | %-12.2f | %-12.2f | %-12.2f | %-10.2f |\n",
       "Elastic Hash",
       elastic_stats ? elastic_stats->avg_probes : 0.0,
       hash_get_avg_insert_probes(elastic_hash),
       hash_get_avg_lookup_probes(elastic_hash),
       hash_get_avg_delete_probes(elastic_hash),
       elastic_stats ? (double)elastic_stats->num_entries / elastic_stats->table_size : 0.0);

// 对其他哈希表类型也做类似修改

// 运行插入基准测试
start_time = get_time_ns();
for (size_t i = 0; i < num_inserts; i++) {
    // 生成随机键值对
    generate_random_key(key, key_size);
    generate_random_value(value, value_size);

    // 插入到哈希表
    hash_insert(elastic_hash, key, key_size, value, value_size);
}
elastic_insert_time = (get_time_ns() - start_time) / num_inserts;

// 运行查找基准测试
start_time = get_time_ns();
for (size_t i = 0; i < num_lookups; i++) {
    // 生成随机键
    generate_random_key(key, key_size);

    // 查找键
    hash_lookup(elastic_hash, key, key_size, NULL);
}
elastic_lookup_time = (get_time_ns() - start_time) / num_lookups;

// 运行删除基准测试
start_time = get_time_ns();
for (size_t i = 0; i < num_deletes; i++) {
    // 生成随机键
    generate_random_key(key, key_size);

    // 删除键
    hash_delete(elastic_hash, key, key_size);
}
elastic_delete_time = (get_time_ns() - start_time) / num_deletes;