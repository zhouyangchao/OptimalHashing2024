#ifndef ABSTRACT_HASH_HPP
#define ABSTRACT_HASH_HPP

#include <string>
#include <stdexcept>

class AbstractHash {
public:
    // 更合理的接口命名
    virtual void insert(const std::string &key, int value) = 0;
    virtual void erase(const std::string &key) = 0;
    virtual int find(const std::string &key) const = 0;
    
    // 可选：默认实现 contains 接口
    virtual bool contains(const std::string &key) const {
        try {
            find(key);
            return true;
        } catch(const std::runtime_error &) {
            return false;
        }
    }
    
    virtual ~AbstractHash() {}
};

#endif // ABSTRACT_HASH_HPP
