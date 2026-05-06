#include <stddef.h>

#include "mtf.h"

static void init_alphabet(unsigned char *alphabet) {
    int i;
    for (i = 0; i < 256; ++i) {
        alphabet[i] = (unsigned char)i;
    }
}

void mtf_encode(unsigned char *input, size_t len, unsigned char *output) {
    unsigned char alphabet[256];
    size_t i;

    init_alphabet(alphabet);

    for (i = 0; i < len; ++i) {
        unsigned char value = input[i];
        unsigned char pos = 0;
        unsigned char j;
        while (pos < 255 && alphabet[pos] != value) {
            ++pos;
        }
        output[i] = pos;

        for (j = pos; j > 0; --j) {
            alphabet[j] = alphabet[j - 1];
        }
        alphabet[0] = value;
    }
}

void mtf_decode(unsigned char *input, size_t len, unsigned char *output) {
    unsigned char alphabet[256];
    size_t i;

    init_alphabet(alphabet);

    for (i = 0; i < len; ++i) {
        unsigned char pos = input[i];
        unsigned char value = alphabet[pos];
        unsigned char j;
        output[i] = value;

        for (j = pos; j > 0; --j) {
            alphabet[j] = alphabet[j - 1];
        }
        alphabet[0] = value;
    }
}
