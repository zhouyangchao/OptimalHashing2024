#include "mph.hpp"
#include <cstdlib>
#include <ctime>
#include <queue>       // 可保留，实际不再使用
#include <vector>
#include <iostream>

// MinimalPerfectHash 构造函数及接口实现
MinimalPerfectHash::MinimalPerfectHash(const vector<string>& keys)
    : keys(keys), n(keys.size()), m(static_cast<int>(keys.size() * 2.0)) {  // 将初始 m 提高到 keys.size()*2.0
    srand((unsigned)time(0));
    int outer_attempt = 0;
    const int max_outer_attempts = 10;  // 增加外层重试次数
    bool success = false;
    while (outer_attempt < max_outer_attempts && !success) {
        for (int attempt = 0; attempt < 200; attempt++) {  // 增加内层尝试次数
            seed1 = rand();
            seed2 = rand();
            g.assign(m, 0);
            if (construct()) {
                success = true;
                break;
            }
        }
        if (!success) {
            m = static_cast<int>(m * 1.5);  // 失败时进一步扩展 m 的大小
            outer_attempt++;
        }
    }
    if (!success)
        throw std::runtime_error("无法构造 acyclic graph");
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
    // 初始化顶点度数
    vector<int> deg(m, 0);
    for (int i = 0; i < m; i++) {
        deg[i] = adj[i].size();
    }
    vector<bool> used(n, false);
    // 使用栈记录（顶点，边编号）的消除顺序
    vector<pair<int, int>> stack;
    // 首先将所有度为1的顶点入栈
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
    // 消除过程，更新相邻顶点的度数
    while (!stack.empty()) {
        auto [v, e_id] = stack.back();
        stack.pop_back();
        const auto &edge = edges[e_id];
        int u = (edge.u == v) ? edge.v : edge.u;
        // 根据论文：设置 g[v] 使得 (g[u] + g[v]) mod n 等于 key_index
        int value = edge.key_index - g[u];
        value %= n;
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