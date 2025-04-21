#ifndef MPH_HPP
#define MPH_HPP

#include <vector>
#include <string>
#include <stdexcept>
using namespace std;

// 用于构造静态最优无冲突 hash 的辅助结构
struct Edge {
    int key_index;
    int u, v;
};

// MinimalPerfectHash 实现参考了 https://arxiv.org/html/2501.02305v2 的改进算法，
// 采用基于栈的消除法来构造 acyclic 图，从而生成 minimal perfect hash 函数。
class MinimalPerfectHash {
public:
    // 构造时传入静态键集，生成 minimal perfect hash
    MinimalPerfectHash(const vector<string>& keys);
    
    // 返回 key 对应的 hash 值（范围 [0, n-1]）
    int hash(const string& key) const;
    
    // 封装操作，分别计算 h1 与 h2，用于对比验证
    int computeH1(const string &key) const;
    int computeH2(const string &key) const;
    int encapsulatedHash(const string &key) const;
    
private:
    vector<string> keys;
    int n, m;
    vector<int> g;
    uint32_t seed1, seed2;
    
    bool construct();
    static uint32_t computeHash(const string &key, uint32_t seed);
};

#endif // MPH_HPP
