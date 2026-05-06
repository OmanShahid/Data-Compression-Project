#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

typedef struct {
    unsigned char *data;
    size_t size;
    size_t original_size;
} Block;

typedef struct {
    Block *blocks;
    int num_blocks;
    size_t block_size;
} BlockManager;

typedef struct {
    char *rotation;
    int index;
} Rotation;

typedef struct {
    unsigned short code;
    unsigned char length;
} HuffmanCode;

typedef struct Node {
    unsigned char symbol;
    int freq;
    struct Node *left;
    struct Node *right;
} HuffmanNode;

#endif
