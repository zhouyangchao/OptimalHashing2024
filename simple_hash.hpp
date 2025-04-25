#ifndef SIMPLE_HASH_HPP
#define SIMPLE_HASH_HPP

#include "abstract_hash.hpp"
#include <string>
#include <vector>
#include <stdexcept>

// SimpleHash 实现传统的散列表，使用链地址法解决冲突
class SimpleHash : public AbstractHash {
public:
    SimpleHash(size_t capacity = 101, bool use_paper_optimization = false);
    
    // 修改方法名称以匹配 AbstractHash 接口
    void insert(const std::string &key, int value) override;
    void erase(const std::string &key) override;
    int find(const std::string &key) const override;
    
    // 公开 hashKey 方法用于测试
    size_t hashKey(const std::string &key) const;
    
    // 获取指定位置的链
    const std::vector<std::pair<std::string, int>>& getChainAt(size_t idx) const;
    
    // 获取特定键的探测次数
    int getProbeCount(const std::string &key) const;
    
private:
    size_t capacity;
    std::vector<std::vector<std::pair<std::string, int>>> table;
    bool use_optimization; // 是否使用论文中的优化
};

#endif // SIMPLE_HASH_HPP
