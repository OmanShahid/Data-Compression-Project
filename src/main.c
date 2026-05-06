#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "bwt.h"
#include "config.h"
#include "rans.h"
#include "mtf.h"
#include "rle.h"

#define MAGIC "BZP2"

typedef struct {
    unsigned char rle1_enabled;
    unsigned char bwt_enabled;
    unsigned char mtf_enabled;
    unsigned char rle2_enabled;
    unsigned char ans_enabled;
} PipelineFlags;

static void print_bytes_preview(const char *label, const unsigned char *buf, size_t len) {
    const size_t max_show = 64;
    size_t show = len < max_show ? len : max_show;
    size_t i;

    printf("%s (len=%lu)\n", label, (unsigned long)len);
    if (len == 0) {
        printf("  <empty>\n");
        return;
    }

    printf("  hex: ");
    for (i = 0; i < show; ++i) {
        printf("%02X ", (unsigned int)buf[i]);
    }
    if (show < len) {
        printf("...");
    }
    printf("\n");

    printf("  asc: ");
    for (i = 0; i < show; ++i) {
        unsigned char c = buf[i];
        if (c >= 32 && c <= 126) {
            putchar((int)c);
        } else {
            putchar('.');
        }
    }
    if (show < len) {
        printf("...");
    }
    printf("\n");
}

static int has_trace_flag(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--trace") == 0 || strcmp(argv[i], "-t") == 0) {
            return 1;
        }
    }
    return 0;
}

static void write_u32(FILE *fp, unsigned int v) {
    unsigned char bytes[4];
    bytes[0] = (unsigned char)((v >> 24) & 0xFFU);
    bytes[1] = (unsigned char)((v >> 16) & 0xFFU);
    bytes[2] = (unsigned char)((v >> 8) & 0xFFU);
    bytes[3] = (unsigned char)(v & 0xFFU);
    fwrite(bytes, 1, 4, fp);
}

static unsigned int read_u32(FILE *fp, int *ok) {
    unsigned char bytes[4];
    if (fread(bytes, 1, 4, fp) != 4) {
        *ok = 0;
        return 0;
    }
    return ((unsigned int)bytes[0] << 24) |
           ((unsigned int)bytes[1] << 16) |
           ((unsigned int)bytes[2] << 8) |
           (unsigned int)bytes[3];
}

static int apply_encode_pipeline(const unsigned char *input, size_t len, PipelineFlags flags, int trace, int block_no,
                                 int *primary_index, unsigned char **out_data, size_t *out_len) {
    unsigned char *buf_a = NULL;
    unsigned char *buf_b = NULL;
    size_t max_a = (len * 6) + 4096;
    size_t max_b = (len * 8) + 8192;
    size_t cur_len = len;

    *primary_index = -1;
    *out_data = NULL;
    *out_len = 0;

    if (max_a == 0) {
        max_a = 4096;
    }
    if (max_b == 0) {
        max_b = 8192;
    }

    buf_a = (unsigned char *)malloc(max_a);
    buf_b = (unsigned char *)malloc(max_b);
    if (buf_a == NULL || buf_b == NULL) {
        free(buf_a);
        free(buf_b);
        return -1;
    }

    memcpy(buf_a, input, len);

    if (trace) {
        printf("\n=== ENCODE BLOCK %d ===\n", block_no);
        print_bytes_preview("input", buf_a, cur_len);
    }

    if (flags.rle1_enabled) {
        size_t tmp_len = 0;
        if (trace) {
            print_bytes_preview("RLE1 input", buf_a, cur_len);
        }
        rle1_encode(buf_a, cur_len, buf_b, &tmp_len);
        if (trace) {
            print_bytes_preview("RLE1 output", buf_b, tmp_len);
        }
        cur_len = tmp_len;
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.bwt_enabled) {
        if (trace) {
            print_bytes_preview("BWT input", buf_a, cur_len);
        }
        bwt_encode(buf_a, cur_len, buf_b, primary_index);
        if (trace) {
            printf("BWT primary_index=%d\n", *primary_index);
            print_bytes_preview("BWT output", buf_b, cur_len);
        }
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.mtf_enabled) {
        if (trace) {
            print_bytes_preview("MTF input", buf_a, cur_len);
        }
        mtf_encode(buf_a, cur_len, buf_b);
        if (trace) {
            print_bytes_preview("MTF output", buf_b, cur_len);
        }
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.rle2_enabled) {
        size_t tmp_len = 0;
        if (trace) {
            print_bytes_preview("RLE2 input", buf_a, cur_len);
        }
        rle2_encode(buf_a, cur_len, buf_b, &tmp_len);
        if (trace) {
            print_bytes_preview("RLE2 output", buf_b, tmp_len);
        }
        cur_len = tmp_len;
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.ans_enabled) {
        size_t tmp_len = 0;
        if (trace) {
            print_bytes_preview("ANS input", buf_a, cur_len);
        }
        rans_encode(buf_a, cur_len, buf_b, &tmp_len);
        if (trace) {
            print_bytes_preview("ANS output", buf_b, tmp_len);
        }
        cur_len = tmp_len;
        memcpy(buf_a, buf_b, cur_len);
    }

    if (trace) {
        print_bytes_preview("final encoded payload", buf_a, cur_len);
    }

    *out_data = (unsigned char *)malloc(cur_len == 0 ? 1 : cur_len);
    if (*out_data == NULL) {
        free(buf_a);
        free(buf_b);
        return -1;
    }
    memcpy(*out_data, buf_a, cur_len);
    *out_len = cur_len;

    free(buf_a);
    free(buf_b);
    return 0;
}

static int apply_decode_pipeline(const unsigned char *input, size_t len, size_t expected_size, PipelineFlags flags, int trace,
                                 int block_no, int primary_index, unsigned char **out_data, size_t *out_len) {
    unsigned char *buf_a = NULL;
    unsigned char *buf_b = NULL;
    size_t max_size = (expected_size * 8) + 16384;
    size_t cur_len = len;

    *out_data = NULL;
    *out_len = 0;
    if (max_size < 16384) {
        max_size = 16384;
    }

    buf_a = (unsigned char *)malloc(max_size);
    buf_b = (unsigned char *)malloc(max_size);
    if (buf_a == NULL || buf_b == NULL) {
        free(buf_a);
        free(buf_b);
        return -1;
    }

    memcpy(buf_a, input, len);

    if (trace) {
        printf("\n=== DECODE BLOCK %d ===\n", block_no);
        printf("expected_output_size=%lu primary_index=%d\n", (unsigned long)expected_size, primary_index);
        print_bytes_preview("encoded payload", buf_a, cur_len);
    }

    if (flags.ans_enabled) {
        size_t tmp_len = 0;
        if (trace) {
            print_bytes_preview("ANS input", buf_a, cur_len);
        }
        rans_decode(buf_a, cur_len, buf_b, &tmp_len);
        if (trace) {
            print_bytes_preview("ANS output", buf_b, tmp_len);
        }
        cur_len = tmp_len;
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.rle2_enabled) {
        size_t tmp_len = 0;
        if (trace) {
            print_bytes_preview("RLE2 input", buf_a, cur_len);
        }
        rle2_decode(buf_a, cur_len, buf_b, &tmp_len);
        if (trace) {
            print_bytes_preview("RLE2 output", buf_b, tmp_len);
        }
        cur_len = tmp_len;
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.mtf_enabled) {
        if (trace) {
            print_bytes_preview("MTF input", buf_a, cur_len);
        }
        mtf_decode(buf_a, cur_len, buf_b);
        if (trace) {
            print_bytes_preview("MTF output", buf_b, cur_len);
        }
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.bwt_enabled) {
        if (trace) {
            print_bytes_preview("BWT input", buf_a, cur_len);
        }
        bwt_decode(buf_a, cur_len, primary_index, buf_b);
        if (trace) {
            print_bytes_preview("BWT output", buf_b, cur_len);
        }
        memcpy(buf_a, buf_b, cur_len);
    }
    if (flags.rle1_enabled) {
        size_t tmp_len = 0;
        if (trace) {
            print_bytes_preview("RLE1 input", buf_a, cur_len);
        }
        rle1_decode(buf_a, cur_len, buf_b, &tmp_len);
        if (trace) {
            print_bytes_preview("RLE1 output", buf_b, tmp_len);
        }
        cur_len = tmp_len;
        memcpy(buf_a, buf_b, cur_len);
    }

    if (trace) {
        print_bytes_preview("final decoded block", buf_a, cur_len);
    }

    *out_data = (unsigned char *)malloc(cur_len == 0 ? 1 : cur_len);
    if (*out_data == NULL) {
        free(buf_a);
        free(buf_b);
        return -1;
    }
    memcpy(*out_data, buf_a, cur_len);
    *out_len = cur_len;

    free(buf_a);
    free(buf_b);
    return 0;
}

static int write_compressed(const char *output_file, BlockManager *manager, PipelineFlags flags, size_t block_size, int trace) {
    FILE *fp = fopen(output_file, "wb");
    int i;

    if (fp == NULL) {
        return -1;
    }

    fwrite(MAGIC, 1, 4, fp);
    fputc(flags.rle1_enabled, fp);
    fputc(flags.bwt_enabled, fp);
    fputc(flags.mtf_enabled, fp);
    fputc(flags.rle2_enabled, fp);
    fputc(flags.ans_enabled, fp);
    write_u32(fp, (unsigned int)block_size);
    write_u32(fp, (unsigned int)manager->num_blocks);

    for (i = 0; i < manager->num_blocks; ++i) {
        unsigned char *encoded = NULL;
        size_t encoded_len = 0;
        int primary_index = -1;
        int rc = apply_encode_pipeline(manager->blocks[i].data, manager->blocks[i].size, flags, trace, i, &primary_index, &encoded,
                                       &encoded_len);
        if (rc != 0) {
            fclose(fp);
            return -1;
        }
        write_u32(fp, (unsigned int)manager->blocks[i].size);
        write_u32(fp, (unsigned int)primary_index);
        write_u32(fp, (unsigned int)encoded_len);
        if (encoded_len > 0) {
            fwrite(encoded, 1, encoded_len, fp);
        }
        free(encoded);
    }

    fclose(fp);
    return 0;
}

static int read_compressed(const char *input_file, const char *output_file, int trace) {
    FILE *fp = fopen(input_file, "rb");
    char magic[4];
    PipelineFlags flags;
    int ok = 1;
    unsigned int block_size;
    unsigned int num_blocks;
    BlockManager *manager;
    unsigned int i;

    if (fp == NULL) {
        return -1;
    }

    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, MAGIC, 4) != 0) {
        fclose(fp);
        return -1;
    }

    flags.rle1_enabled = (unsigned char)fgetc(fp);
    flags.bwt_enabled = (unsigned char)fgetc(fp);
    flags.mtf_enabled = (unsigned char)fgetc(fp);
    flags.rle2_enabled = (unsigned char)fgetc(fp);
    flags.ans_enabled = (unsigned char)fgetc(fp);

    block_size = read_u32(fp, &ok);
    num_blocks = read_u32(fp, &ok);
    if (!ok) {
        fclose(fp);
        return -1;
    }

    manager = (BlockManager *)calloc(1, sizeof(BlockManager));
    if (manager == NULL) {
        fclose(fp);
        return -1;
    }
    manager->block_size = block_size;
    manager->num_blocks = (int)num_blocks;
    manager->blocks = (Block *)calloc(num_blocks, sizeof(Block));
    if (manager->blocks == NULL) {
        free(manager);
        fclose(fp);
        return -1;
    }

    for (i = 0; i < num_blocks; ++i) {
        unsigned int original_size = read_u32(fp, &ok);
        int primary_index = (int)read_u32(fp, &ok);
        unsigned int payload_len = read_u32(fp, &ok);
        unsigned char *payload;
        unsigned char *decoded = NULL;
        size_t decoded_len = 0;
        if (!ok) {
            free_block_manager(manager);
            fclose(fp);
            return -1;
        }

        payload = (unsigned char *)malloc(payload_len == 0 ? 1 : payload_len);
        if (payload == NULL) {
            free_block_manager(manager);
            fclose(fp);
            return -1;
        }
        if (payload_len > 0 && fread(payload, 1, payload_len, fp) != payload_len) {
            free(payload);
            free_block_manager(manager);
            fclose(fp);
            return -1;
        }

        if (apply_decode_pipeline(payload, payload_len, original_size, flags, trace, (int)i, primary_index, &decoded, &decoded_len) != 0) {
            free(payload);
            free_block_manager(manager);
            fclose(fp);
            return -1;
        }
        free(payload);

        manager->blocks[i].data = decoded;
        manager->blocks[i].size = decoded_len;
        manager->blocks[i].original_size = original_size;
    }

    if (reassemble_blocks(manager, output_file) != 0) {
        free_block_manager(manager);
        fclose(fp);
        return -1;
    }

    free_block_manager(manager);
    fclose(fp);
    return 0;
}

static void print_usage(const char *program_name) {
    printf("Usage:\n");
    printf("  %s c <input_file> <output_file> [config.ini] [--trace|-t]\n", program_name);
    printf("  %s d <input_file> <output_file> [--trace|-t]\n", program_name);
}

int main(int argc, char **argv) {
    int trace = has_trace_flag(argc, argv);
    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }

    if (argv[1][0] == 'c') {
        ProjectConfig cfg;
        const char *config_path = (argc >= 5) ? argv[4] : "config.ini";
        BlockManager *manager;
        PipelineFlags flags;

        if (load_config(config_path, &cfg) != 0) {
            set_default_config(&cfg);
        }

        manager = divide_into_blocks(argv[2], cfg.block_size);
        if (manager == NULL) {
            fprintf(stderr, "failed to read input file: %s\n", argv[2]);
            return 1;
        }

        flags.rle1_enabled = (unsigned char)cfg.rle1_enabled;
        flags.bwt_enabled = 1;
        flags.mtf_enabled = (unsigned char)cfg.mtf_enabled;
        flags.rle2_enabled = (unsigned char)cfg.rle2_enabled;
        flags.ans_enabled = (unsigned char)cfg.ans_enabled;

        if (write_compressed(argv[3], manager, flags, cfg.block_size, trace) != 0) {
            fprintf(stderr, "compression failed\n");
            free_block_manager(manager);
            return 1;
        }

        free_block_manager(manager);
        printf("compressed successfully: %s\n", argv[3]);
    } else if (argv[1][0] == 'd') {
        if (read_compressed(argv[2], argv[3], trace) != 0) {
            fprintf(stderr, "decompression failed\n");
            return 1;
        }
        printf("decompressed successfully: %s\n", argv[3]);
    } else {
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
