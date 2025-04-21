#ifndef ELASTIC_HASH_HPP
#define ELASTIC_HASH_HPP

#include "abstract_hash.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>

struct Bucket {
    int local_depth; // The depth of the bucket in the directory
    std::vector<std::pair<std::string, int>> entries; // Key-value pairs stored in the bucket
};

class ElasticHash : public AbstractHash {
public:
    ElasticHash(int bucket_size = 4); // Constructor to initialize the hash table with a given bucket size
    
    void insert(const std::string &key, int value) override; // Add a key-value pair to the hash table
    void erase(const std::string &key) override; // Delete a key from the hash table
    int find(const std::string &key) const override; // Find the value associated with a key
    
private:
    int bucket_size; // Maximum number of entries in a bucket
    int global_depth; // Global depth of the directory
    std::vector<Bucket*> directory; // Directory pointing to buckets
    
    int hashKey(const std::string &key) const; // Hash function to compute the index for a key
    void splitBucket(int index); // Split a bucket when it overflows
    Bucket* getBucket(int index) const; // Get the bucket corresponding to a directory index
    void doubleDirectory(); // Double the size of the directory when needed
};

#endif // ELASTIC_HASH_HPP
