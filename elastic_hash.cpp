#include "elastic_hash.hpp"
#include <functional>

ElasticHash::ElasticHash(int bucket_size)
    : bucket_size(bucket_size), global_depth(1) {
    directory.resize(1 << global_depth, nullptr);
    for (int i = 0; i < (1 << global_depth); i++) {
        directory[i] = new Bucket{global_depth, {}};
    }
}

int ElasticHash::hashKey(const std::string &key) const {
    std::hash<std::string> hasher;
    return static_cast<int>(hasher(key));
}

Bucket* ElasticHash::getBucket(int index) const {
    return directory[index];
}

void ElasticHash::doubleDirectory() {
    int old_size = directory.size();
    global_depth++;
    directory.resize(1 << global_depth);
    for (int i = 0; i < old_size; i++) {
        directory[i + old_size] = directory[i];
    }
}

void ElasticHash::splitBucket(int index) {
    Bucket* bucket = getBucket(index);
    int local_depth = bucket->local_depth;
    if (local_depth == global_depth) {
        doubleDirectory();
    }
    Bucket* newBucket = new Bucket{local_depth + 1, {}};
    bucket->local_depth++;
    
    // 重新分配当前 bucket 中项
    std::vector<std::pair<std::string, int>> temp = bucket->entries;
    bucket->entries.clear();
    int mask = (1 << bucket->local_depth) - 1;
    for (auto &entry : temp) {
        int hash_val = hashKey(entry.first);
        int dir_index = hash_val & mask;
        if ((dir_index & (1 << (bucket->local_depth - 1))) != 0)
            newBucket->entries.push_back(entry);
        else
            bucket->entries.push_back(entry);
    }
    // 更新目录中指向 bucket 的指针
    int dir_size = directory.size();
    for (int i = 0; i < dir_size; i++) {
        int idx_mask = i & mask;
        if (directory[i] == bucket) {
            if ((idx_mask & (1 << (bucket->local_depth - 1))) != 0)
                directory[i] = newBucket;
        }
    }
}

void ElasticHash::insert(const std::string &key, int value) {
    int hash_val = hashKey(key);
    int dir_index = hash_val & ((1 << global_depth) - 1);
    Bucket* bucket = getBucket(dir_index);
    // 如果 key 存在则更新
    for (auto &entry : bucket->entries) {
        if (entry.first == key) {
            entry.second = value;
            return;
        }
    }
    if (bucket->entries.size() >= (unsigned)bucket_size) {
        splitBucket(dir_index);
        insert(key, value);
        return;
    }
    bucket->entries.push_back({key, value});
}

void ElasticHash::erase(const std::string &key) {
    int hash_val = hashKey(key);
    int dir_index = hash_val & ((1 << global_depth) - 1);
    Bucket* bucket = getBucket(dir_index);
    for (auto it = bucket->entries.begin(); it != bucket->entries.end(); ++it) {
        if (it->first == key) {
            bucket->entries.erase(it);
            return;
        }
    }
    throw std::runtime_error("Key not found in ElasticHash");
}

int ElasticHash::find(const std::string &key) const {
    int hash_val = hashKey(key);
    int dir_index = hash_val & ((1 << global_depth) - 1);
    Bucket* bucket = getBucket(dir_index);
    for (auto &entry : bucket->entries) {
        if (entry.first == key) {
            return entry.second;
        }
    }
    throw std::runtime_error("Key not found in ElasticHash");
}
