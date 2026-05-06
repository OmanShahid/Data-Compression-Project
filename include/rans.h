#ifndef RANS_H
#define RANS_H

#include <stddef.h>

// The scaling factor for probabilities. 
// 4096 (12 bits) is a very standard and fast scale for rANS.
#define RANS_SCALE_BITS 12
#define RANS_SCALE (1 << RANS_SCALE_BITS)

// We need 32-bit state variables
typedef unsigned int RansState;

// Structure to hold our probability model
typedef struct {
    int freq[256];        // How often each symbol appears
    int start[256];       // Cumulative frequency (starting point in the range)
    int max_freq;         // Used for optimization
} RansModel;

// Public functions matching your pipeline architecture
void build_rans_model(unsigned char *input, size_t len, RansModel *model);
void rans_encode(unsigned char *input, size_t len, unsigned char *output, size_t *out_len);
void rans_decode(unsigned char *input, size_t len, unsigned char *output, size_t *out_len);

#endif