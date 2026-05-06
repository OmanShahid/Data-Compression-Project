#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"

BlockManager *divide_into_blocks(const char *filename, size_t block_size);
int reassemble_blocks(BlockManager *manager, const char *output_filename);
void free_block_manager(BlockManager *manager);

#endif
