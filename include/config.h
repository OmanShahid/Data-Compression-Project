#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

typedef struct {
    size_t block_size;
    int rle1_enabled;
    char bwt_type[32];
    int mtf_enabled;
    int rle2_enabled;
    int ans_enabled;
    int benchmark_mode;
    int output_metrics;
    char input_directory[256];
    char output_directory[256];
} ProjectConfig;

int load_config(const char *filename, ProjectConfig *config);
void set_default_config(ProjectConfig *config);
int parse_bool(const char *value);

#endif
