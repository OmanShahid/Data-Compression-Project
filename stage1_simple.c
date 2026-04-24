#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MIN_BLOCK_SIZE 102400UL
#define MAX_BLOCK_SIZE 921600UL

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
    size_t block_size;
    int rle1_enabled;
    char bwt_type[32];
} Config;

typedef struct {
    char *rotation;
    int index;
} Rotation;

/* ---------------- CONFIG ---------------- */

void set_default_config(Config *config) {
    config->block_size = 500000UL;
    config->rle1_enabled = 1;
    strcpy(config->bwt_type, "matrix");
}

char *trim(char *s) {
    char *end;

    while (*s && isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '\0') {
        return s;
    }

    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return s;
}

void remove_comment(char *s) {
    while (*s) {
        if (*s == '#' || *s == ';') {
            *s = '\0';
            return;
        }
        s++;
    }
}

int parse_bool(const char *value) {
    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        return 1;
    }
    if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
        return 0;
    }
    return -1;
}

int load_config(const char *filename, Config *config) {
    FILE *fp;
    char line[512];

    set_default_config(config);

    fp = fopen(filename, "r");
    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *eq;
        char *key;
        char *value;
        char *p = trim(line);

        remove_comment(p);
        p = trim(p);

        if (*p == '\0') {
            continue;
        }

        if (*p == '[') {
            continue;
        }

        eq = strchr(p, '=');
        if (eq == NULL) {
            continue;
        }

        *eq = '\0';
        key = trim(p);
        value = trim(eq + 1);

        if (strcmp(key, "block_size") == 0) {
            config->block_size = (size_t)strtoul(value, NULL, 10);
        } else if (strcmp(key, "rle1_enabled") == 0) {
            int b = parse_bool(value);
            if (b != -1) {
                config->rle1_enabled = b;
            }
        } else if (strcmp(key, "bwt_type") == 0) {
            strncpy(config->bwt_type, value, sizeof(config->bwt_type) - 1);
            config->bwt_type[sizeof(config->bwt_type) - 1] = '\0';
        }
    }

    fclose(fp);

    if (config->block_size < MIN_BLOCK_SIZE || config->block_size > MAX_BLOCK_SIZE) {
        return -1;
    }

    return 0;
}

/* ---------------- BLOCK HANDLING ---------------- */

BlockManager *divide_into_blocks(const char *filename, size_t block_size) {
    FILE *fp;
    BlockManager *manager;
    int capacity = 4;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    manager = (BlockManager *)malloc(sizeof(BlockManager));
    if (manager == NULL) {
        fclose(fp);
        return NULL;
    }

    manager->blocks = (Block *)malloc(capacity * sizeof(Block));
    manager->num_blocks = 0;
    manager->block_size = block_size;

    if (manager->blocks == NULL) {
        free(manager);
        fclose(fp);
        return NULL;
    }

    while (1) {
        unsigned char *buffer;
        size_t bytes_read;

        buffer = (unsigned char *)malloc(block_size);
        if (buffer == NULL) {
            fclose(fp);
            return NULL;
        }

        bytes_read = fread(buffer, 1, block_size, fp);

        if (bytes_read == 0) {
            free(buffer);
            break;
        }

        if (manager->num_blocks == capacity) {
            Block *new_blocks;
            capacity *= 2;
            new_blocks = (Block *)realloc(manager->blocks, capacity * sizeof(Block));
            if (new_blocks == NULL) {
                free(buffer);
                fclose(fp);
                return NULL;
            }
            manager->blocks = new_blocks;
        }

        if (bytes_read < block_size) {
            unsigned char *small = (unsigned char *)realloc(buffer, bytes_read);
            if (small != NULL) {
                buffer = small;
            }
        }

        manager->blocks[manager->num_blocks].data = buffer;
        manager->blocks[manager->num_blocks].size = bytes_read;
        manager->blocks[manager->num_blocks].original_size = bytes_read;
        manager->num_blocks++;

        if (bytes_read < block_size) {
            break;
        }
    }

    fclose(fp);
    return manager;
}

int reassemble_blocks(BlockManager *manager, const char *output_filename) {
    FILE *fp;
    int i;

    fp = fopen(output_filename, "wb");
    if (fp == NULL) {
        return -1;
    }

    for (i = 0; i < manager->num_blocks; i++) {
        if (manager->blocks[i].size > 0) {
            fwrite(manager->blocks[i].data, 1, manager->blocks[i].size, fp);
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

    for (i = 0; i < manager->num_blocks; i++) {
        free(manager->blocks[i].data);
    }

    free(manager->blocks);
    free(manager);
}

/* ---------------- RLE-1 ---------------- */
/* Simple binary-safe format:
   [byte][count] [byte][count] ...
   count is 1..255
*/

void rle1_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;

    while (i < len) {
        unsigned char ch = input[i];
        unsigned char count = 1;

        while (i + count < len && input[i + count] == ch && count < 255) {
            count++;
        }

        output[j++] = ch;
        output[j++] = count;
        i += count;
    }

    *out_len = j;
}

void rle1_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;

    while (i + 1 < len) {
        unsigned char ch = input[i++];
        unsigned char count = input[i++];

        while (count--) {
            output[j++] = ch;
        }
    }

    *out_len = j;
}

/* ---------------- BWT ---------------- */

static unsigned char *global_doubled = NULL;
static size_t global_len = 0;

int compare_rotations(const void *a, const void *b) {
    const Rotation *ra = (const Rotation *)a;
    const Rotation *rb = (const Rotation *)b;
    return memcmp(global_doubled + ra->index, global_doubled + rb->index, global_len);
}

void bwt_encode(unsigned char *input, size_t len,
                unsigned char *output, int *primary_index) {
    Rotation *rotations;
    unsigned char *doubled;
    size_t i;

    if (len == 0) {
        *primary_index = 0;
        return;
    }

    rotations = (Rotation *)malloc(len * sizeof(Rotation));
    doubled = (unsigned char *)malloc(2 * len);

    memcpy(doubled, input, len);
    memcpy(doubled + len, input, len);

    global_doubled = doubled;
    global_len = len;

    for (i = 0; i < len; i++) {
        rotations[i].rotation = (char *)(doubled + i);
        rotations[i].index = (int)i;
    }

    qsort(rotations, len, sizeof(Rotation), compare_rotations);

    for (i = 0; i < len; i++) {
        int start = rotations[i].index;
        int last = (start + (int)len - 1) % (int)len;
        output[i] = input[last];

        if (start == 0) {
            *primary_index = (int)i;
        }
    }

    free(rotations);
    free(doubled);
    global_doubled = NULL;
    global_len = 0;
}

void bwt_decode(unsigned char *input, size_t len,
                int primary_index, unsigned char *output) {
    int count[256] = {0};
    int start[256] = {0};
    int occ[256] = {0};
    int *next;
    size_t i;
    int sum = 0;
    int row;

    if (len == 0) {
        return;
    }

    next = (int *)malloc(len * sizeof(int));
    if (next == NULL) {
        return;
    }

    for (i = 0; i < len; i++) {
        count[input[i]]++;
    }

    for (i = 0; i < 256; i++) {
        start[i] = sum;
        sum += count[i];
    }

    for (i = 0; i < len; i++) {
        unsigned char c = input[i];
        next[i] = start[c] + occ[c];
        occ[c]++;
    }

    row = primary_index;

    for (i = len; i > 0; i--) {
        output[i - 1] = input[row];
        row = next[row];
    }

    free(next);
}

/* ---------------- HELPERS ---------------- */

unsigned char *copy_buffer(const unsigned char *src, size_t len) {
    unsigned char *dst;

    if (len == 0) {
        return NULL;
    }

    dst = (unsigned char *)malloc(len);
    if (dst == NULL) {
        return NULL;
    }

    memcpy(dst, src, len);
    return dst;
}

int process_block(Block *block, Config *config) {
    unsigned char *original;
    unsigned char *rle_data = NULL;
    unsigned char *bwt_data = NULL;
    unsigned char *bwt_back = NULL;
    unsigned char *decoded = NULL;

    size_t rle_len = 0;
    size_t decoded_len = 0;
    int primary_index = 0;

    original = copy_buffer(block->data, block->size);
    if (block->size > 0 && original == NULL) {
        return -1;
    }

    /* Step 1: RLE encode */
    if (config->rle1_enabled) {
        rle_data = (unsigned char *)malloc(block->size * 2 + 2);
        if (rle_data == NULL) {
            free(original);
            return -1;
        }

        rle1_encode(block->data, block->size, rle_data, &rle_len);
    } else {
        rle_data = copy_buffer(block->data, block->size);
        rle_len = block->size;
    }

    if (rle_len > 0 && rle_data == NULL) {
        free(original);
        return -1;
    }

    /* Step 2: BWT encode */
    bwt_data = (unsigned char *)malloc(rle_len + 1);
    bwt_back = (unsigned char *)malloc(rle_len + 1);

    if ((rle_len > 0) && (bwt_data == NULL || bwt_back == NULL)) {
        free(original);
        free(rle_data);
        free(bwt_data);
        free(bwt_back);
        return -1;
    }

    bwt_encode(rle_data, rle_len, bwt_data, &primary_index);

    /* Step 3: BWT decode */
    bwt_decode(bwt_data, rle_len, primary_index, bwt_back);

    /* Step 4: RLE decode */
    decoded = (unsigned char *)malloc(block->original_size + 1);
    if (decoded == NULL) {
        free(original);
        free(rle_data);
        free(bwt_data);
        free(bwt_back);
        return -1;
    }

    if (config->rle1_enabled) {
        rle1_decode(bwt_back, rle_len, decoded, &decoded_len);
    } else {
        memcpy(decoded, bwt_back, rle_len);
        decoded_len = rle_len;
    }

    /* Step 5: verify */
    if (decoded_len != block->original_size ||
        memcmp(original, decoded, decoded_len) != 0) {
        free(original);
        free(rle_data);
        free(bwt_data);
        free(bwt_back);
        free(decoded);
        return -1;
    }

    free(block->data);
    block->data = decoded;
    block->size = decoded_len;

    free(original);
    free(rle_data);
    free(bwt_data);
    free(bwt_back);

    return 0;
}

/* ---------------- MAIN ---------------- */

int main(int argc, char *argv[]) {
    const char *input_file;
    const char *output_file;
    const char *config_file = "config.ini";

    Config config;
    BlockManager *manager;
    int i;

    if (argc < 3 || argc > 4) {
        printf("Usage: %s input_file output_file [config.ini]\n", argv[0]);
        return 1;
    }

    input_file = argv[1];
    output_file = argv[2];

    if (argc == 4) {
        config_file = argv[3];
    }

    if (load_config(config_file, &config) != 0) {
        printf("Error: could not read config file.\n");
        return 1;
    }

    if (strcmp(config.bwt_type, "matrix") != 0) {
        printf("Error: this simple Stage 1 version supports only bwt_type = matrix\n");
        return 1;
    }

    manager = divide_into_blocks(input_file, config.block_size);
    if (manager == NULL) {
        printf("Error: could not divide file into blocks.\n");
        return 1;
    }

    for (i = 0; i < manager->num_blocks; i++) {
        if (process_block(&manager->blocks[i], &config) != 0) {
            printf("Error: block %d failed Stage 1 verification.\n", i);
            free_block_manager(manager);
            return 1;
        }
    }

    if (reassemble_blocks(manager, output_file) != 0) {
        printf("Error: could not write output file.\n");
        free_block_manager(manager);
        return 1;
    }

    printf("Stage 1 completed successfully.\n");
    printf("Input was divided into %d block(s).\n", manager->num_blocks);
    printf("Output file created: %s\n", output_file);

    free_block_manager(manager);
    return 0;
}