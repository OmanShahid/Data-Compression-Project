#include <stdlib.h>
#include <string.h>

#include "bwt.h"

// Structure needed for Prefix Doubling Suffix Array
typedef struct {
    int index;
    int rank[2];
} Suffix;

// Compare function for qsort to sort by the two rank halves
static int compare_suffixes(const void *a, const void *b) {
    Suffix *s1 = (Suffix *)a;
    Suffix *s2 = (Suffix *)b;
    if (s1->rank[0] != s2->rank[0]) {
        return s1->rank[0] - s2->rank[0];
    }
    return s1->rank[1] - s2->rank[1];
}

/*
 * Builds suffix array for efficient BWT using Prefix Doubling
 * @param text: Input text
 * @param n: Length of text
 * @return: Suffix array
 */
static int *build_suffix_array(unsigned char *text, int n) {
    int *sa = (int *)malloc(n * sizeof(int));
    Suffix *suffixes = (Suffix *)malloc(n * sizeof(Suffix));
    int *ind2rank = (int *)malloc(n * sizeof(int));
    
    // Variables declared at the top for C89 compatibility
    int i, k, rank, prev_rank, next_index;

    if (!sa || !suffixes || !ind2rank) {
        free(sa);
        free(suffixes);
        free(ind2rank);
        return NULL;
    }

    // 1. Initial ranking based on the first character and the next (wrapped) character
    for (i = 0; i < n; i++) {
        suffixes[i].index = i;
        suffixes[i].rank[0] = text[i];
        suffixes[i].rank[1] = text[(i + 1) % n]; // Cyclic wrap-around for BWT
    }

    qsort(suffixes, n, sizeof(Suffix), compare_suffixes);

    // 2. Prefix Doubling: sort by lengths of 4, 8, 16...
    for (k = 4; k < 2 * n; k = k * 2) {
        rank = 0;
        prev_rank = suffixes[0].rank[0];
        suffixes[0].rank[0] = rank;
        ind2rank[suffixes[0].index] = 0;

        // Assign new ranks to all suffixes based on the previous sort
        for (i = 1; i < n; i++) {
            if (suffixes[i].rank[0] == prev_rank &&
                suffixes[i].rank[1] == suffixes[i - 1].rank[1]) {
                prev_rank = suffixes[i].rank[0];
                suffixes[i].rank[0] = rank;
            } else {
                prev_rank = suffixes[i].rank[0];
                suffixes[i].rank[0] = ++rank;
            }
            ind2rank[suffixes[i].index] = i;
        }

        // Assign the next half's rank for the next iteration (with cyclic wrap-around)
        for (i = 0; i < n; i++) {
            next_index = (suffixes[i].index + k / 2) % n;
            suffixes[i].rank[1] = suffixes[ind2rank[next_index]].rank[0];
        }

        qsort(suffixes, n, sizeof(Suffix), compare_suffixes);
    }

    // 3. Extract the final sorted indices
    for (i = 0; i < n; i++) {
        sa[i] = suffixes[i].index;
    }

    free(suffixes);
    free(ind2rank);
    return sa;
}

void bwt_encode(unsigned char *input, size_t len, unsigned char *output, int *primary_index) {
    int *sa;
    size_t i;

    if (len == 0) {
        *primary_index = 0;
        return;
    }

    // Use the optimized Suffix Array algorithm instead of naive matrix sorting
    sa = build_suffix_array(input, (int)len);
    if (sa == NULL) {
        *primary_index = -1;
        return;
    }

    // Extract the BWT output from the sorted suffix array
    for (i = 0; i < len; ++i) {
        int start = sa[i];
        int prev = (start + (int)len - 1) % (int)len;
        output[i] = input[(size_t)prev];
        if (start == 0) {
            *primary_index = (int)i;
        }
    }

    free(sa);
}

void bwt_decode(unsigned char *input, size_t len, int primary_index, unsigned char *output) {
    size_t i;
    int counts[256] = {0};
    int totals[256];
    int *ranks;
    int *next;
    int idx;

    if (len == 0) {
        return;
    }

    ranks = (int *)malloc(sizeof(int) * len);
    next = (int *)malloc(sizeof(int) * len);
    if (ranks == NULL || next == NULL) {
        free(ranks);
        free(next);
        return;
    }

    for (i = 0; i < len; ++i) {
        unsigned char c = input[i];
        ranks[i] = counts[c];
        counts[c]++;
    }

    totals[0] = 0;
    for (i = 1; i < 256; ++i) {
        totals[i] = totals[i - 1] + counts[i - 1];
    }

    for (i = 0; i < len; ++i) {
        unsigned char c = input[i];
        next[i] = totals[c] + ranks[i];
    }

    idx = primary_index;
    for (i = len; i > 0; --i) {
        output[i - 1] = input[(size_t)idx];
        idx = next[idx];
    }

    free(ranks);
    free(next);
}