#include "funnel_hash.hpp"

FunnelHash::FunnelHash() {
    // 可根据需求初始化
}

void FunnelHash::insert(const std::string &key, int value) {
    map[key] = value;
}

void FunnelHash::erase(const std::string &key) {
    if (map.erase(key) == 0)
        throw std::runtime_error("Key not found in FunnelHash");
}

int FunnelHash::find(const std::string &key) const {
    auto it = map.find(key);
    if (it == map.end())
        throw std::runtime_error("Key not found in FunnelHash");
    return it->second;
}
