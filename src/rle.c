#include <stdlib.h>
#include <string.h>
#include "rle.h"

void rle1_encode(const unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;
    const size_t threshold = 4;

    while (i < len) {
        size_t run_len = 1;
        while (i + run_len < len && input[i + run_len] == input[i] && run_len < 255) {
            run_len++;
        }

        if (run_len >= threshold) {
            output[j++] = 0xAA; 
            output[j++] = (unsigned char)run_len;
            output[j++] = input[i];
            i += run_len;
        } else {
            // FIX: Use a local variable for the loop so 'i' isn't corrupted
            size_t k;
            for (k = 0; k < run_len; k++) {
                if (input[i] == 0xAA) {
                    output[j++] = 0xAA;
                    output[j++] = 0; 
                } else {
                    output[j++] = input[i];
                }
                i++; // This correctly advances 'i' for the outer while loop
            }
        }
    }
    *out_len = j;
}

void rle1_decode(unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;

    while (i < len) {
        if (input[i] == 0xAA) {
            // We found a marker! Check the next byte
            unsigned char next_byte = input[++i];
            
            if (next_byte == 0) {
                // It was an escaped literal 0xAA
                output[j++] = 0xAA;
                i++;
            } else {
                // It's a run: [Marker][Length][Value]
                unsigned char run_len = next_byte;
                unsigned char value = input[++i];
                for (unsigned int k = 0; k < run_len; ++k) {
                    output[j++] = value;
                }
                i++;
            }
        } else {
            // Just a normal literal byte
            output[j++] = input[i++];
        }
    }

    *out_len = j;
}

void rle2_encode(unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;

    while (i < len) {
        if (input[i] == 0) {
            unsigned int run = 1;
            while ((i + run) < len && input[i + run] == 0 && run < 255U) {
                ++run;
            }
            output[j++] = 0;
            output[j++] = (unsigned char)run;
            i += run;
        } else {
            output[j++] = 1;
            output[j++] = input[i];
            i += 1;
        }
    }

    *out_len = j;
}

void rle2_decode(unsigned char *input, size_t len, unsigned char *output, size_t *out_len) {
    size_t i = 0;
    size_t j = 0;

    // Use i < len to ensure we don't stop prematurely
    while (i < len) {
        unsigned char tag = input[i++];
        
        // If a tag exists but the value byte is missing, the data is corrupt
        if (i >= len) break; 
        
        unsigned char value = input[i++];
        
        if (tag == 0) {
            // Run of zeros
            unsigned int k;
            for (k = 0; k < value; ++k) {
                output[j++] = 0;
            }
        } else {
            // Literal value
            output[j++] = value;
        }
    }

    *out_len = j;
}
