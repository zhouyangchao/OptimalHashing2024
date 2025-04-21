#include "simple_hash.hpp"
#include <functional>

SimpleHash::SimpleHash(size_t capacity)
    : capacity(capacity) {
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
    table[idx].push_back({key, value});
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
