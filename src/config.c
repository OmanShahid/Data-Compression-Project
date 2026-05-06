#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

static void trim(char *s) {
    char *start = s;
    char *end;
    while (*start && isspace((unsigned char)*start)) {
        ++start;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) {
        --end;
    }
    *end = '\0';
}

int parse_bool(const char *value) {
    char lower[16];
    size_t i;
    if (value == NULL) {
        return 0;
    }
    for (i = 0; i < sizeof(lower) - 1 && value[i] != '\0'; ++i) {
        lower[i] = (char)tolower((unsigned char)value[i]);
    }
    lower[i] = '\0';
    if (strcmp(lower, "true") == 0 || strcmp(lower, "1") == 0 || strcmp(lower, "yes") == 0) {
        return 1;
    }
    return 0;
}

void set_default_config(ProjectConfig *config) {
    config->block_size = 500000;
    config->rle1_enabled = 1;
    strcpy(config->bwt_type, "matrix");
    config->mtf_enabled = 1;
    config->rle2_enabled = 1;
    config->ans_enabled = 1;
    config->benchmark_mode = 0;
    config->output_metrics = 1;
    strcpy(config->input_directory, "./benchmarks/");
    strcpy(config->output_directory, "./results/");
}

int load_config(const char *filename, ProjectConfig *config) {
    FILE *fp = fopen(filename, "r");
    char line[512];
    char current_section[64] = "";

    if (config == NULL) {
        return -1;
    }
    set_default_config(config);

    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *eq;
        char *key;
        char *value;
        char *comment = strchr(line, '#');

        if (comment != NULL) {
            *comment = '\0';
        }

        trim(line);
        if (line[0] == '\0') {
            continue;
        }

        if (line[0] == '[') {
            char *rbr = strchr(line, ']');
            if (rbr != NULL) {
                *rbr = '\0';
                strncpy(current_section, line + 1, sizeof(current_section) - 1);
                current_section[sizeof(current_section) - 1] = '\0';
            }
            continue;
        }

        eq = strchr(line, '=');
        if (eq == NULL) {
            continue;
        }
        *eq = '\0';
        key = line;
        value = eq + 1;
        trim(key);
        trim(value);

        if (strcmp(current_section, "General") == 0) {
            if (strcmp(key, "block_size") == 0) {
                size_t bs = (size_t)strtoul(value, NULL, 10);
                if (bs >= 100000 && bs <= 900000) {
                    config->block_size = bs;
                }
            } else if (strcmp(key, "rle1_enabled") == 0) {
                config->rle1_enabled = parse_bool(value);
            } else if (strcmp(key, "bwt_type") == 0) {
                strncpy(config->bwt_type, value, sizeof(config->bwt_type) - 1);
                config->bwt_type[sizeof(config->bwt_type) - 1] = '\0';
            } else if (strcmp(key, "mtf_enabled") == 0) {
                config->mtf_enabled = parse_bool(value);
            } else if (strcmp(key, "rle2_enabled") == 0) {
                config->rle2_enabled = parse_bool(value);
            } else if (strcmp(key, "ans_enabled") == 0) {
                config->ans_enabled = parse_bool(value);
            }
        } else if (strcmp(current_section, "Performance") == 0) {
            if (strcmp(key, "benchmark_mode") == 0) {
                config->benchmark_mode = parse_bool(value);
            } else if (strcmp(key, "output_metrics") == 0) {
                config->output_metrics = parse_bool(value);
            }
        } else if (strcmp(current_section, "Paths") == 0) {
            if (strcmp(key, "input_directory") == 0) {
                strncpy(config->input_directory, value, sizeof(config->input_directory) - 1);
                config->input_directory[sizeof(config->input_directory) - 1] = '\0';
            } else if (strcmp(key, "output_directory") == 0) {
                strncpy(config->output_directory, value, sizeof(config->output_directory) - 1);
                config->output_directory[sizeof(config->output_directory) - 1] = '\0';
            }
        }
    }

    fclose(fp);
    return 0;
}
