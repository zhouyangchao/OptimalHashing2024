#include <iostream>
#include <vector>
#include <string>
#include <random>
#include "mph.hpp"
#include "simple_hash.hpp"
#include "elastic_hash.hpp"
#include "funnel_hash.hpp"
#include <chrono>

using namespace std;

// 生成固定长度的随机字符串
string random_string(size_t length, mt19937 &rng) {
    static const string chars = "abcdefghijklmnopqrstuvwxyz";
    uniform_int_distribution<> dist(0, chars.size()-1);
    string s;
    for (size_t i = 0; i < length; i++) {
        s.push_back(chars[dist(rng)]);
    }
    return s;
}

int main() {
    // 固定随机种子，生成 100 个随机字符串，长度为 5
    mt19937 rng(42);
    vector<string> keys;
    for (int i = 0; i < 100; i++) {
        keys.push_back(random_string(5, rng));
    }
    
    // 输出部分随机生成的键以供观察
    cout << "Randomly generated keys (first 10):" << endl;
    for (int i = 0; i < 10; i++) {
        cout << keys[i] << " ";
    }
    cout << "\n\n";
    
    // MinimalPerfectHash（静态构造，只支持查询）
    MinimalPerfectHash mph(keys);
    cout << "Testing MinimalPerfectHash (static):" << endl;
    for (const auto &key : keys) {
        cout << key << " -> " << mph.hash(key) << endl;
    }
    
    // 使用 mph.hash(key) 作为示例 value
    cout << "\nUsing mph.hash(key) as sample value." << endl;
    
    // SimpleHash（动态散列）
    SimpleHash sh(101);
    cout << "\nTesting SimpleHash (dynamic):" << endl;
    for (const auto &key : keys) {
        int value = mph.hash(key);
        sh.insert(key, value);
    }
    // 输出前 10 项
    cout << "Initial contents (first 10):" << endl;
    for (int i = 0; i < 10; i++) {
        cout << keys[i] << " -> " << sh.find(keys[i]) << endl;
    }
    // 动态更新测试：插入新键和删除已有键
    sh.insert("zzzzz", mph.hash("zzzzz"));
    sh.erase(keys[3]); // 删除第 4 个原始键
    cout << "\nSimpleHash Update:" << endl;
    cout << "After inserting 'zzzzz' and erasing '" << keys[3] << "':" << endl;
    for (const auto &key : vector<string>{"zzzzz", keys[3]}) {
        cout << key << " -> ";
        try { cout << sh.find(key) << endl; }
        catch(const std::runtime_error &e) { cout << "Not found" << endl; }
    }
    
    // ElasticHash（动态散列）
    ElasticHash eh(4);
    cout << "\nTesting ElasticHash (dynamic):" << endl;
    for (const auto &key : keys) {
        int value = mph.hash(key);
        eh.insert(key, value);
    }
    cout << "Initial sample (first 10):" << endl;
    for (int i = 0; i < 10; i++) {
        cout << keys[i] << " -> " << eh.find(keys[i]) << endl;
    }
    // 动态更新测试
    eh.insert("zzzzz", mph.hash("zzzzz"));
    eh.erase(keys[5]); // 删除第 6 个键
    cout << "\nElasticHash Update:" << endl;
    cout << "After inserting 'zzzzz' and erasing '" << keys[5] << "':" << endl;
    for (const auto &key : vector<string>{"zzzzz", keys[5]}) {
        cout << key << " -> ";
        try { cout << eh.find(key) << endl; }
        catch(const std::runtime_error &e) { cout << "Not found" << endl; }
    }
    
    // FunnelHash（动态散列）
    FunnelHash fh;
    cout << "\nTesting FunnelHash (dynamic):" << endl;
    for (const auto &key : keys) {
        int value = mph.hash(key);
        fh.insert(key, value);
    }
    cout << "Initial sample (first 10):" << endl;
    for (int i = 0; i < 10; i++) {
        cout << keys[i] << " -> " << fh.find(keys[i]) << endl;
    }
    // 动态更新测试
    fh.insert("zzzzz", mph.hash("zzzzz"));
    fh.erase(keys[7]); // 删除第 8 个键
    cout << "\nFunnelHash Update:" << endl;
    cout << "After inserting 'zzzzz' and erasing '" << keys[7] << "':" << endl;
    for (const auto &key : vector<string>{"zzzzz", keys[7]}) {
        cout << key << " -> ";
        try { cout << fh.find(key) << endl; }
        catch(const std::runtime_error &e) { cout << "Not found" << endl; }
    }
    
    return 0;
}
