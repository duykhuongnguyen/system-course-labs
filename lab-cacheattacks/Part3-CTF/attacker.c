#include "util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

int main(int argc, char const *argv[]) {
    int flag = -1;

    // Put your capture-the-flag code here
    uint8_t *buf = mmap(NULL, 8 * 1024 * 1024,  // 8MB buffer
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        -1, 0);
    if (buf == MAP_FAILED) {
    perror("mmap");
    exit(1);
    }

    uint64_t max_latency = 0;
    int best_set = -1;

    // Measure each set
    for (int set = 0; set < 2048; set++) {  // 2048 sets in L2
        uint64_t total_time = 0;

        for (int i = 0; i < 5; i++) {  // Average over 5 measurements
            volatile uint8_t *addr = buf + (set * 4096); // 1 set per 4KB
            total_time += measure_one_block_access_time((uint64_t)addr);
        }

        uint64_t avg_time = total_time / 5;

        if (avg_time > max_latency) {
            max_latency = avg_time;
            best_set = set;
        }
    }

    flag = best_set;

    printf("Flag: %d\n", flag);
    return 0;
}