#ifndef FUNNEL_HASH_HPP
#define FUNNEL_HASH_HPP

#include "abstract_hash.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_map>

class FunnelHash : public AbstractHash {
public:
    FunnelHash();
    
    // 修改接口名称：insert/erase/find
    void insert(const std::string &key, int value) override;
    void erase(const std::string &key) override;
    int find(const std::string &key) const override;
    
private:
    std::unordered_map<std::string, int> map;
};

#endif // FUNNEL_HASH_HPP
