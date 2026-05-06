#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rans.h"

// --------------------------------------------------------
// Model Building
// --------------------------------------------------------

void build_rans_model(unsigned char *input, size_t len, RansModel *model) {
    size_t i;
    int c;
    int raw_freq[256] = {0};

    // 1. Count actual frequencies
    for (i = 0; i < len; ++i) {
        raw_freq[input[i]]++;
    }

    // 2. Scale frequencies to exactly equal RANS_SCALE (4096)
    // This is required for the rANS math to work perfectly.
    int total = 0;
    for (c = 0; c < 256; ++c) {
        if (raw_freq[c] > 0) {
            // Simple scaling (ensure at least 1 if it appeared)
            long long scaled = ((long long)raw_freq[c] * RANS_SCALE) / len;
            model->freq[c] = (int)(scaled == 0 ? 1 : scaled);
            total += model->freq[c];
        } else {
            model->freq[c] = 0;
        }
    }

    // Fix rounding errors to make total exactly RANS_SCALE
    int diff = RANS_SCALE - total;
    if (diff != 0) {
        // Find the most frequent symbol to absorb the error
        int max_c = 0;
        for (c = 1; c < 256; ++c) {
            if (model->freq[c] > model->freq[max_c]) {
                max_c = c;
            }
        }
        model->freq[max_c] += diff;
    }

    // 3. Calculate cumulative frequencies (starts)
    int current_start = 0;
    for (c = 0; c < 256; ++c) {
        model->start[c] = current_start;
        current_start += model->freq[c];
    }
}

// --------------------------------------------------------
// Encoding (Working Backwards!)
// --------------------------------------------------------

static void put_u32(unsigned char *output, size_t *index, unsigned int value) {
    output[(*index)++] = (unsigned char)((value >> 24) & 0xFFU);
    output[(*index)++] = (unsigned char)((value >> 16) & 0xFFU);
    output[(*index)++] = (unsigned char)((value >> 8) & 0xFFU);
    output[(*index)++] = (unsigned char)(value & 0xFFU);
}

void rans_encode(unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    RansModel model;
    build_rans_model(input, len, &model);

    // Initial state needs a solid starting point (usually lower bound)
    RansState state = (1 << 16); 
    size_t out_idx = 0;
    int c;

    // We must write the header so the decoder knows the probabilities
    put_u32(output, &out_idx, (unsigned int)len); // Original length is crucial
    for (c = 0; c < 256; ++c) {
        // Writing 2 bytes per frequency since scale is 4096
        output[out_idx++] = (unsigned char)((model.freq[c] >> 8) & 0xFF);
        output[out_idx++] = (unsigned char)(model.freq[c] & 0xFF);
    }

    // Crucial: Allocate a temporary buffer for backwards encoding
    unsigned char *temp_out = (unsigned char *)malloc(len * 2);
    size_t temp_idx = 0;

    // ENCODE BACKWARDS
    long long i;
    for (i = len - 1; i >= 0; --i) {
        unsigned char sym = input[i];
        int freq = model.freq[sym];
        int start = model.start[sym];

        // FIX 1: Correct rANS byte-flush threshold to prevent 32-bit overflow
        unsigned int max_val = ((1U << 24) / (unsigned int)RANS_SCALE) * (unsigned int)freq;
        while (state >= max_val) {
            temp_out[temp_idx++] = (unsigned char)(state & 0xFF);
            state >>= 8;
        }

        // 2. The rANS Encoding Step
        state = (state / freq) * RANS_SCALE + (state % freq) + start;
    }

    // FIX 2: Write final state DIRECTLY to output so it is NOT reversed by the loop
    put_u32(output, &out_idx, state);

    // Because we encoded backwards, we must copy the temp buffer 
    // into the final output buffer IN REVERSE ORDER.
    for (i = (long long)temp_idx - 1; i >= 0; --i) {
        output[out_idx++] = temp_out[i];
    }

    *out_len = out_idx;
    free(temp_out);
}

// --------------------------------------------------------
// Decoding (Working Forwards)
// --------------------------------------------------------

static unsigned int read_u32(const unsigned char *input, size_t *index) {
    unsigned int v = 0;
    v |= (unsigned int)input[(*index)++] << 24;
    v |= (unsigned int)input[(*index)++] << 16;
    v |= (unsigned int)input[(*index)++] << 8;
    v |= (unsigned int)input[(*index)++];
    return v;
}

void rans_decode(unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    if (len < 516) { // Header must be at least length + 256*2
        *out_len = 0;
        return;
    }

    size_t in_idx = 0;
    unsigned int expected_len = read_u32(input, &in_idx);
    
    RansModel model;
    int current_start = 0;
    int c;

    // Read header to rebuild the model
    for (c = 0; c < 256; ++c) {
        int f = (input[in_idx] << 8) | input[in_idx + 1];
        in_idx += 2;
        model.freq[c] = f;
        model.start[c] = current_start;
        current_start += f;
    }

    // Read the initial state we saved at the end of encoding
    RansState state = read_u32(input, &in_idx);
    size_t produced = 0;

    // DECODE FORWARDS
    while (produced < expected_len) {
        // 1. Find which symbol corresponds to the current slot
        int slot = state % RANS_SCALE;
        unsigned char sym = 0;
        
        // Find symbol by scanning starts (can be optimized with a lookup table)
        for (c = 1; c < 256; ++c) {
            if (model.start[c] > slot) {
                break;
            }
            sym = (unsigned char)c;
        }

        output[produced++] = sym;

        // 2. The rANS Decoding Step
        state = (state / RANS_SCALE) * model.freq[sym] + slot - model.start[sym];

        // 3. Refill state if it gets too small
        while (state < (1 << 16) && in_idx < len) {
            state = (state << 8) | input[in_idx++];
        }
    }

    *out_len = expected_len;
}