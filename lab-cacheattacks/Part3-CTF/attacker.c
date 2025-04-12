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

    uint64_t set_scores[2048] = {0}; // Accumulate timing scores per set

    int rounds = 20; // How many full scans of all sets
    int samples_per_set = 5; // How many samples per set in each round

    for (int round = 0; round < rounds; round++) {
        for (int set = 0; set < 2048; set++) {
            uint64_t total_time = 0;

            for (int i = 0; i < samples_per_set; i++) {
                volatile uint8_t *addr = buf + (set * 4096); // 1 set per 4KB
                total_time += measure_one_block_access_time((uint64_t)addr);
            }

            uint64_t avg_time = total_time / samples_per_set;
            set_scores[set] += avg_time; // Accumulate score
        }
    }

    // Find the set with maximum accumulated score
    uint64_t max_score = 0;
    int best_set = -1;

    for (int set = 0; set < 2048; set++) {
        if (set_scores[set] > max_score) {
            max_score = set_scores[set];
            best_set = set;
        }
    }

    flag = best_set;

    printf("Flag: %d\n", flag);
    return 0;
}