#include "mph.hpp"
#include <cstdlib>
#include <ctime>
#include <queue>       // 可保留，实际不再使用
#include <vector>
#include <iostream>
#include <chrono>

// MinimalPerfectHash 构造函数及接口实现
MinimalPerfectHash::MinimalPerfectHash(const vector<string>& keys)
    : keys(keys), n(keys.size()) {
    // Use a much larger m for better acyclic graph probability
    m = static_cast<int>(keys.size() * 3.0);
    
    // For empty key set, nothing to do
    if (n == 0) {
        construction_time = 0;
        return;
    }
    
    auto start_time = chrono::high_resolution_clock::now();
    
    // Fixed seed pool with better hash properties
    vector<uint32_t> seed_pool = {
        0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210,
        0xC3B2A190, 0x5A6B7C8D, 0x12345678, 0x87654321,
        0xABCDEF01, 0x9E3779B9, 0xBF58476D, 0x1F0A3942
    };
    
    bool success = false;
    srand((unsigned)time(0));
    
    // Use multiple strategies to try to construct the MPH
    for (int strategy = 0; strategy < 3 && !success; strategy++) {
        // Strategy 1: Use predefined seed pool
        // Strategy 2: Increase m size
        // Strategy 3: Use completely random seeds
        
        if (strategy == 1) {
            m = static_cast<int>(m * 1.5);  // Increase size for second attempt
        }
        
        for (int attempt = 0; attempt < 50; attempt++) {
            // Fix signedness comparison warning
            if (strategy == 0 && attempt < static_cast<int>(seed_pool.size() - 1)) {
                // Strategy 1: Use predefined seeds
                seed1 = seed_pool[attempt % seed_pool.size()];
                seed2 = seed_pool[(attempt + 1) % seed_pool.size()];
            } else if (strategy == 2) {
                // Strategy 3: Completely random seeds
                seed1 = rand() ^ (rand() << 15);
                seed2 = rand() ^ (rand() << 15);
            } else {
                // Mix strategies
                if (attempt % 3 == 0) {
                    seed1 = rand() ^ (rand() << 15);
                    seed2 = rand() ^ (rand() << 15);
                } else {
                    seed1 = seed_pool[attempt % seed_pool.size()];
                    seed2 = seed_pool[(attempt + seed_pool.size()/2) % seed_pool.size()];
                }
            }
            
            g.assign(m, 0);
            if (construct()) {
                success = true;
                break;
            }
        }
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    construction_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
    
    if (!success) {
        // Instead of throwing an exception, we'll use a fallback method
        // We'll assign each key a unique value in [0, n-1] based on its position in the keys vector
        // This ensures that we at least have a functioning (though not perfect) hash
        m = n;
        g.assign(m, 0);
        seed1 = 12345;
        seed2 = 67890;
        cout << "Warning: Using fallback hash implementation (not MPH) for " << n << " keys" << endl;
    }
}

// 新版构造实现：采用栈结构消除法，参考论文v2改进方案
bool MinimalPerfectHash::construct() {
    vector<Edge> edges;
    edges.reserve(n);
    vector<vector<int>> adj(m);
    // 为每个 key 构造边
    for (int i = 0; i < n; i++) {
        int u = computeHash(keys[i], seed1) % m;
        int v = computeHash(keys[i], seed2) % m;
        edges.push_back({i, u, v});
        adj[u].push_back(i);
        if (u != v)
            adj[v].push_back(i);
    }
    // 优化：直接使用度数表，跳过度数为0的顶点
    vector<int> deg(m, 0);
    for (int i = 0; i < m; i++) {
        deg[i] = adj[i].size();
    }
    vector<bool> used(n, false);
    // 使用栈记录（顶点，边编号）的消除顺序
    vector<pair<int, int>> stack;
    // 找出所有度为1的顶点
    for (int v = 0; v < m; v++) {
        if (deg[v] == 1) {
            for (int ei : adj[v]) {
                if (!used[ei]) {
                    stack.push_back({v, ei});
                    used[ei] = true;
                    break;
                }
            }
        }
    }
    // 优化的压缩和赋值过程
    while (!stack.empty()) {
        // 修复C++11不支持结构化绑定的问题
        pair<int, int> stack_item = stack.back();
        int v = stack_item.first;
        int e_id = stack_item.second;
        stack.pop_back();
        const auto &edge = edges[e_id];
        int u = (edge.u == v) ? edge.v : edge.u;
        // 根据论文：设置 g[v] 使得 (g[u] + g[v]) mod n 等于 key_index
        int value = (edge.key_index - g[u]) % n;
        if (value < 0)
            value += n;
        g[v] = value;
        // 更新相邻顶点 u 的度数
        deg[u]--;
        if (deg[u] == 1) {
            for (int eid : adj[u]) {
                if (!used[eid]) {
                    stack.push_back({u, eid});
                    used[eid] = true;
                    break;
                }
            }
        }
    }
    // 检查是否所有边都已被消除
    for (bool flag : used) {
        if (!flag)
            return false;
    }
    return true;
}

int MinimalPerfectHash::hash(const string &key) const {
    // If we're using the fallback implementation, do a simple hash to get a value in range
    if (m == n && g[0] == 0 && n > 0) {
        uint32_t h = computeHash(key, seed1);
        for (size_t i = 0; i < keys.size(); i++) {
            if (keys[i] == key) return i;
        }
        return h % n; // Fallback for keys not in the original set
    }
    
    // Normal MPH implementation
    int h1 = computeHash(key, seed1) % m;
    int h2 = computeHash(key, seed2) % m;
    return (g[h1] + g[h2]) % n;
}

int MinimalPerfectHash::computeH1(const string &key) const {
    return computeHash(key, seed1) % m;
}

int MinimalPerfectHash::computeH2(const string &key) const {
    return computeHash(key, seed2) % m;
}

int MinimalPerfectHash::encapsulatedHash(const string &key) const {
    int h1 = computeH1(key);
    int h2 = computeH2(key);
    return (g[h1] + g[h2]) % n;
}

uint32_t MinimalPerfectHash::computeHash(const string &key, uint32_t seed) {
    uint32_t h = seed;
    for (char c : key) {
        h = h * 31 + static_cast<unsigned char>(c);
    }
    return h;
}