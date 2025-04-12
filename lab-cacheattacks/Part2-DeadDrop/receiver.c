
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>


int main(int argc, char **argv)
{
    // 1. Covert Channel Setup
    uint8_t *buf = mmap(NULL, 2 * 1024 * 1024, 
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                        -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    volatile uint8_t *target = buf + 4096 * 0; // pick one cache set (e.g., set 0)

    printf("Please press enter.\n");

    char text_buf[2];
    fgets(text_buf, sizeof(text_buf), stdin);

    printf("Receiver now listening.\n");

    bool listening = true;
    while (listening) {

        uint64_t time = measure_one_block_access_time((uint64_t)target);

        int received_bit;
        uint64_t threshold = 20;

        if (time > threshold) {
            received_bit = 1;
        } else {
            received_bit = 0;
        }

        printf("%d\n", received_bit);

    }

    printf("Receiver finished.\n");

    return 0;
}

