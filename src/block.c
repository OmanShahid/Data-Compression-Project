#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"

BlockManager *divide_into_blocks(const char *filename, size_t block_size) {
    FILE *fp;
    long file_size;
    size_t bytes_to_read;
    int block_count;
    int i;
    BlockManager *manager;

    if (filename == NULL || block_size == 0) {
        return NULL;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    block_count = (int)((file_size + (long)block_size - 1L) / (long)block_size);
    if (block_count == 0) {
        block_count = 1;
    }

    manager = (BlockManager *)calloc(1, sizeof(BlockManager));
    if (manager == NULL) {
        fclose(fp);
        return NULL;
    }

    manager->blocks = (Block *)calloc((size_t)block_count, sizeof(Block));
    if (manager->blocks == NULL) {
        free(manager);
        fclose(fp);
        return NULL;
    }

    manager->num_blocks = block_count;
    manager->block_size = block_size;

    for (i = 0; i < block_count; ++i) {
        long remaining = file_size - (long)((size_t)i * block_size);
        bytes_to_read = remaining > (long)block_size ? block_size : (size_t)(remaining > 0 ? remaining : 0);
        if (file_size == 0) {
            bytes_to_read = 0;
        }

        manager->blocks[i].data = (unsigned char *)malloc(bytes_to_read == 0 ? 1 : bytes_to_read);
        if (manager->blocks[i].data == NULL) {
            free_block_manager(manager);
            fclose(fp);
            return NULL;
        }

        manager->blocks[i].size = bytes_to_read;
        manager->blocks[i].original_size = bytes_to_read;

        if (bytes_to_read > 0) {
            size_t got = fread(manager->blocks[i].data, 1, bytes_to_read, fp);
            if (got != bytes_to_read) {
                free_block_manager(manager);
                fclose(fp);
                return NULL;
            }
        }
    }

    fclose(fp);
    return manager;
}

int reassemble_blocks(BlockManager *manager, const char *output_filename) {
    FILE *fp;
    int i;

    if (manager == NULL || output_filename == NULL) {
        return -1;
    }

    fp = fopen(output_filename, "wb");
    if (fp == NULL) {
        return -1;
    }

    for (i = 0; i < manager->num_blocks; ++i) {
        if (manager->blocks[i].size > 0) {
            if (fwrite(manager->blocks[i].data, 1, manager->blocks[i].size, fp) != manager->blocks[i].size) {
                fclose(fp);
                return -1;
            }
        }
    }

    fclose(fp);
    return 0;
}

void free_block_manager(BlockManager *manager) {
    int i;

    if (manager == NULL) {
        return;
    }

    if (manager->blocks != NULL) {
        for (i = 0; i < manager->num_blocks; ++i) {
            free(manager->blocks[i].data);
            manager->blocks[i].data = NULL;
            manager->blocks[i].size = 0;
            manager->blocks[i].original_size = 0;
        }
        free(manager->blocks);
        manager->blocks = NULL;
    }

    manager->num_blocks = 0;
    manager->block_size = 0;
    free(manager);
}
