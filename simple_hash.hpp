#ifndef SIMPLE_HASH_HPP
#define SIMPLE_HASH_HPP

#include "abstract_hash.hpp"
#include <string>
#include <vector>
#include <stdexcept>

// SimpleHash 实现传统的散列表，使用链地址法解决冲突
class SimpleHash : public AbstractHash {
public:
    SimpleHash(size_t capacity = 101);
    
    // 修改方法名称以匹配 AbstractHash 接口
    void insert(const std::string &key, int value) override;
    void erase(const std::string &key) override;
    int find(const std::string &key) const override;
    
private:
    size_t capacity;
    std::vector<std::vector<std::pair<std::string, int>>> table;
    size_t hashKey(const std::string &key) const;
};

#endif // SIMPLE_HASH_HPP
