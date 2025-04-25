#include "simple_hash.hpp"
#include <functional>

SimpleHash::SimpleHash(size_t capacity, bool use_paper_optimization)
    : capacity(capacity), use_optimization(use_paper_optimization) {
    table.resize(capacity);
}

size_t SimpleHash::hashKey(const std::string &key) const {
    return std::hash<std::string>{}(key) % capacity;
}

void SimpleHash::insert(const std::string &key, int value) {
    size_t idx = hashKey(key);
    for (auto &pair : table[idx]) {
        if (pair.first == key) {
            pair.second = value;
            return;
        }
    }
    
    // 使用论文中的优化策略，在插入时优化链表排序
    if (use_optimization && !table[idx].empty()) {
        // 根据访问频率优化排序（简化版本）
        table[idx].insert(table[idx].begin(), {key, value});
    } else {
        table[idx].push_back({key, value});
    }
}

void SimpleHash::erase(const std::string &key) {
    size_t idx = hashKey(key);
    for (auto it = table[idx].begin(); it != table[idx].end(); ++it) {
        if (it->first == key) {
            table[idx].erase(it);
            return;
        }
    }
    throw std::runtime_error("Key not found in erase");
}

int SimpleHash::find(const std::string &key) const {
    size_t idx = hashKey(key);
    for (const auto &pair : table[idx]) {
        if (pair.first == key) {
            return pair.second;
        }
    }
    throw std::runtime_error("Key not found in find");
}

// 新增方法实现

const std::vector<std::pair<std::string, int>>& SimpleHash::getChainAt(size_t idx) const {
    return table[idx];
}

int SimpleHash::getProbeCount(const std::string &key) const {
    size_t idx = hashKey(key);
    int probes = 1;
    
    for (const auto &pair : table[idx]) {
        if (pair.first == key) {
            return probes;
        }
        probes++;
    }
    
    return probes;
}
