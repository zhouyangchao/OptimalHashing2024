#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

#include "../include/hash_ops.h"  // Unified hash table interface

// Test parameters
typedef struct {
    size_t table_size;           // Hash table size
    size_t num_operations;       // Number of operations to perform
    double insert_ratio;         // Ratio of insert operations
    double lookup_ratio;         // Ratio of lookup operations
    double delete_ratio;         // Ratio of delete operations
    double load_factor;          // Target load factor
    uint32_t seed;               // Random seed
    hash_type_t hash_type;       // Hash table type to test
    bool verbose;                // Whether to output detailed information
} test_params_t;

// Test results
typedef struct {
    double avg_insert_time;      // Average insert time (ns)
    double avg_lookup_time;      // Average lookup time (ns)
    double avg_delete_time;      // Average delete time (ns)
    uint64_t total_probes;       // Total probe count
    double avg_probes;           // Average probe count
    double load_factor;          // Final load factor

    // Add these fields:
    double avg_insert_probes;    // Average probes for insert operations
    double avg_lookup_probes;    // Average probes for lookup operations
    double avg_delete_probes;    // Average probes for delete operations
} test_results_t;

// Function prototype
void print_comparison_table(test_results_t* elastic_results, test_results_t* funnel_results,
                           test_results_t* linear_results, test_results_t* uniform_results);

// Get current time in microseconds
uint64_t get_time_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Generate random key
void generate_random_key(char* buffer, size_t size, uint32_t* seed) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = 'a' + (rand_r(seed) % 26);
    }
}

// Run benchmark
test_results_t run_benchmark(test_params_t* params) {
    test_results_t results = {0};
    generic_hash_t* hash_table = NULL;
    double delta = 0.05; // Error parameter for elastic and funnel hash

    // Create appropriate hash table based on type
    // Use the appropriate parameter (delta or load_factor) based on hash type
    double param = (params->hash_type == HASH_TYPE_ELASTIC || params->hash_type == HASH_TYPE_FUNNEL)
                   ? delta : params->load_factor;

    hash_table = hash_create(params->hash_type, params->table_size, param, NULL);

    if (!hash_table) {
        fprintf(stderr, "Failed to create hash table\n");
        return results;
    }

    // Generate test data
    char** keys = (char**)malloc(params->num_operations * sizeof(char*));
    char** values = (char**)malloc(params->num_operations * sizeof(char*));
    uint32_t seed = params->seed;

    for (size_t i = 0; i < params->num_operations; i++) {
        // Use fixed-size keys and values for simplicity
        keys[i] = (char*)malloc(16);
        values[i] = (char*)malloc(16);
        generate_random_key(keys[i], 15, &seed);
        generate_random_key(values[i], 15, &seed);
        keys[i][15] = '\0';
        values[i][15] = '\0';
    }

    // Track time and operations
    uint64_t insert_time = 0;
    uint64_t lookup_time = 0;
    uint64_t delete_time = 0;
    size_t num_inserts = 0;
    size_t num_lookups = 0;
    size_t num_deletes = 0;

    // Perform operations
    for (size_t i = 0; i < params->num_operations; i++) {
        double r = (double)rand_r(&seed) / RAND_MAX;

        if (r < params->insert_ratio) {
            // Perform insert
            uint64_t start = get_time_usec();
            bool success = false;

            success = hash_insert(hash_table, keys[i], strlen(keys[i]), values[i], strlen(values[i]));

            uint64_t end = get_time_usec();

            if (success) {
                insert_time += (end - start);
                num_inserts++;
            }
        } else if (r < params->insert_ratio + params->lookup_ratio) {
            // Perform lookup
            size_t lookup_size = 0;
            uint64_t start = get_time_usec();
            hash_lookup(hash_table, keys[i], strlen(keys[i]), &lookup_size);
            uint64_t end = get_time_usec();
            lookup_time += (end - start);
            num_lookups++;
        } else {
            // Perform delete
            uint64_t start = get_time_usec();
            bool success = false;

            // Use a random key for deletion (may or may not have been inserted before)
            size_t key_idx = rand_r(&seed) % params->num_operations;

            success = hash_delete(hash_table, keys[key_idx], strlen(keys[key_idx]));

            uint64_t end = get_time_usec();

            if (success) {
                delete_time += (end - start);
                num_deletes++;
            }
        }
    }

    // Calculate results
    if (num_inserts > 0) results.avg_insert_time = (double)insert_time * 1000 / num_inserts; // ns
    if (num_lookups > 0) results.avg_lookup_time = (double)lookup_time * 1000 / num_lookups; // ns
    if (num_deletes > 0) results.avg_delete_time = (double)delete_time * 1000 / num_deletes; // ns

    // Get statistics
    hash_stats_t* stats = hash_get_stats(hash_table);
    if (stats) {
        results.total_probes = stats->total_probes;
        results.avg_probes = stats->avg_probes;
        results.load_factor = hash_get_load_factor(hash_table);

        // Get probe counts by operation type
        results.avg_insert_probes = hash_get_avg_insert_probes(hash_table);
        results.avg_lookup_probes = hash_get_avg_lookup_probes(hash_table);
        results.avg_delete_probes = hash_get_avg_delete_probes(hash_table);
    } else {
        results.total_probes = 0;
        results.avg_probes = 0.0;
        results.load_factor = 0.0;
        results.avg_insert_probes = 0.0;
        results.avg_lookup_probes = 0.0;
        results.avg_delete_probes = 0.0;
    }

    // Free resources
    for (size_t i = 0; i < params->num_operations; i++) {
        free(keys[i]);
        free(values[i]);
    }
    free(keys);
    free(values);

    hash_destroy(hash_table);

    return results;
}

// Print test results (table format)
void print_results(const char* name, test_results_t* results) {
    // Print header row if this is the first result
    static bool header_printed = false;
    if (!header_printed) {
        printf("+------------------+------------------+------------------+------------------+------------------+\n");
        printf("| %-16s | %-16s | %-16s | %-16s | %-16s |\n",
               "Hash Type", "Avg Probes", "Insert Probes", "Lookup Probes", "Delete Probes");
        printf("+------------------+------------------+------------------+------------------+------------------+\n");
        header_printed = true;
    }

    // Print this result's row with consistent spacing
    printf("| %-16s | %-16.2f | %-16.2f | %-16.2f | %-16.2f |\n",
           name,
           results->avg_probes,
           results->avg_insert_probes,
           results->avg_lookup_probes,
           results->avg_delete_probes);
}

// Print usage information
void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -s, --size SIZE       Set hash table size (default: 100000)\n");
    printf("  -o, --ops COUNT       Set number of operations (default: 50000)\n");
    printf("  -l, --load FACTOR     Set load factor (default: 0.75)\n");
    printf("  -i, --insert RATIO    Set insert operation ratio (default: 0.7)\n");
    printf("  -u, --lookup RATIO    Set lookup operation ratio (default: 0.2)\n");
    printf("  -d, --delete RATIO    Set delete operation ratio (default: 0.1)\n");
    printf("  -r, --seed SEED       Set random seed (default: current time)\n");
    printf("  -v, --verbose         Enable verbose output\n");
    printf("  -h, --help            Display this help and exit\n");
    printf("\nNote: Insert, lookup and delete ratios should sum to 1.0\n");
}

// Parse command line arguments
bool parse_args(int argc, char* argv[], test_params_t* params) {
    static struct option long_options[] = {
        {"size",    required_argument, 0, 's'},
        {"ops",     required_argument, 0, 'o'},
        {"load",    required_argument, 0, 'l'},
        {"insert",  required_argument, 0, 'i'},
        {"lookup",  required_argument, 0, 'u'},
        {"delete",  required_argument, 0, 'd'},
        {"seed",    required_argument, 0, 'r'},
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "s:o:l:i:u:d:r:vh",
                             long_options, &option_index)) != -1) {
        switch (opt) {
            case 's':
                params->table_size = strtoul(optarg, NULL, 10);
                break;
            case 'o':
                params->num_operations = strtoul(optarg, NULL, 10);
                break;
            case 'l':
                params->load_factor = atof(optarg);
                if (params->load_factor <= 0.0 || params->load_factor >= 1.0) {
                    fprintf(stderr, "Error: Load factor must be between 0.0 and 1.0\n");
                    return false;
                }
                break;
            case 'i':
                params->insert_ratio = atof(optarg);
                if (params->insert_ratio < 0.0 || params->insert_ratio > 1.0) {
                    fprintf(stderr, "Error: Insert ratio must be between 0.0 and 1.0\n");
                    return false;
                }
                break;
            case 'u':
                params->lookup_ratio = atof(optarg);
                if (params->lookup_ratio < 0.0 || params->lookup_ratio > 1.0) {
                    fprintf(stderr, "Error: Lookup ratio must be between 0.0 and 1.0\n");
                    return false;
                }
                break;
            case 'd':
                params->delete_ratio = atof(optarg);
                if (params->delete_ratio < 0.0 || params->delete_ratio > 1.0) {
                    fprintf(stderr, "Error: Delete ratio must be between 0.0 and 1.0\n");
                    return false;
                }
                break;
            case 'r':
                params->seed = strtoul(optarg, NULL, 10);
                break;
            case 'v':
                params->verbose = true;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_usage(argv[0]);
                return false;
        }
    }

    // Validate that ratios sum to approximately 1.0
    double sum = params->insert_ratio + params->lookup_ratio + params->delete_ratio;
    if (sum < 0.99 || sum > 1.01) {
        fprintf(stderr, "Error: Insert, lookup, and delete ratios should sum to 1.0\n");
        fprintf(stderr, "Current sum: %.2f\n", sum);
        return false;
    }

    return true;
}

// Main function
int main(int argc, char* argv[]) {
    // Default parameters
    test_params_t params = {
        .table_size = 100000,
        .num_operations = 50000,
        .insert_ratio = 0.7,
        .lookup_ratio = 0.2,
        .delete_ratio = 0.1,
        .load_factor = 0.75,
        .seed = (uint32_t)time(NULL),
        .hash_type = HASH_TYPE_ELASTIC,
        .verbose = false
    };

    // Parse command line arguments
    if (!parse_args(argc, argv, &params)) {
        return EXIT_FAILURE;
    }

    // Run all hash table types for comparison
    test_results_t elastic_results, funnel_results, linear_results, uniform_results;

    printf("Running benchmark...\n");
    printf("Table size: %zu\n", params.table_size);
    printf("Operation count: %zu\n", params.num_operations);
    printf("Load factor: %.2f\n", params.load_factor);
    printf("Operation ratios: Insert=%.2f, Lookup=%.2f, Delete=%.2f\n\n",
           params.insert_ratio, params.lookup_ratio, params.delete_ratio);

    // Run elastic hash test
    params.hash_type = HASH_TYPE_ELASTIC;
    elastic_results = run_benchmark(&params);
    print_results("Elastic Hash", &elastic_results);

    // Run funnel hash test
    params.hash_type = HASH_TYPE_FUNNEL;
    funnel_results = run_benchmark(&params);
    print_results("Funnel Hash", &funnel_results);

    // Run linear probing test
    params.hash_type = HASH_TYPE_LINEAR;
    linear_results = run_benchmark(&params);
    print_results("Linear Probing", &linear_results);

    // Run uniform hash test
    params.hash_type = HASH_TYPE_UNIFORM;
    uniform_results = run_benchmark(&params);
    print_results("Uniform Hash", &uniform_results);

    // Complete the table with updated column count
    printf("+------------------+------------------+------------------+------------------+------------------+\n\n");

    // Output comparison results also in table format
    print_comparison_table(&elastic_results, &funnel_results, &linear_results, &uniform_results);

    return 0;
}

// After running all the benchmarks, print the comparative table
void print_comparison_table(test_results_t* elastic_results, test_results_t* funnel_results,
                           test_results_t* linear_results, test_results_t* uniform_results) {
    printf("\n=========== Performance Comparison (Relative to Linear Probing) ===========\n");
    printf("+------------------+--------------+--------------+------------+\n");
    printf("| %-16s | %-12s | %-12s | %-10s |\n",
           "Hash Type", "Probes (x)", "Insert (x)", "Lookup (x)");
    printf("+------------------+--------------+--------------+------------+\n");

    printf("| %-16s | %-12.2f | %-12.2f | %-10.2f |\n",
           "Elastic Hash",
           linear_results->avg_probes / elastic_results->avg_probes,
           linear_results->avg_insert_probes / elastic_results->avg_insert_probes,
           linear_results->avg_lookup_probes / elastic_results->avg_lookup_probes);

    printf("| %-16s | %-12.2f | %-12.2f | %-10.2f |\n",
           "Funnel Hash",
           linear_results->avg_probes / funnel_results->avg_probes,
           linear_results->avg_insert_probes / funnel_results->avg_insert_probes,
           linear_results->avg_lookup_probes / funnel_results->avg_lookup_probes);

    printf("| %-16s | %-12.2f | %-12.2f | %-10.2f |\n",
           "Uniform Hash",
           linear_results->avg_probes / uniform_results->avg_probes,
           linear_results->avg_insert_probes / uniform_results->avg_insert_probes,
           linear_results->avg_lookup_probes / uniform_results->avg_lookup_probes);

    printf("| %-16s | %-12.2f | %-12.2f | %-10.2f |\n",
           "Linear Probing", 1.00, 1.00, 1.00);

    printf("+------------------+--------------+--------------+------------+\n");
}