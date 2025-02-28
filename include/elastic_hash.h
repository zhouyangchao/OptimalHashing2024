#ifndef ELASTIC_HASH_H
#define ELASTIC_HASH_H

#include "common.h"
#include "hash_ops.h"

// Function to get operations interface
const hash_ops_t* elastic_hash_get_ops(void);

#endif /* ELASTIC_HASH_H */