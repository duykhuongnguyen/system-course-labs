
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>



int main(int argc, char **argv)
{
    uint8_t *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                        -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    volatile uint8_t *target = buf + 4096 * 0; // Same target as sender

    printf("Please press enter.\n");
    char text_buf[2];
    fgets(text_buf, sizeof(text_buf), stdin);
    printf("Receiver now listening.\n");

    uint64_t threshold = 20; // threshold to distinguish 1 and 0
    bool start_detected = false;

    // Step 1: Wait for start signal
    while (!start_detected) {
        uint64_t time = measure_one_block_access_time((uint64_t)target);
        if (time > threshold) {
            start_detected = true;
        }

        // Small busy-wait delay (no function)
        for (volatile int i = 0; i < 10000; i++);
    }

    printf("Start detected!\n");

    // Step 2: Receive 8 bits
    int received_value = 0;
    for (int i = 7; i >= 0; i--) {
        uint64_t time = measure_one_block_access_time((uint64_t)target);

        int received_bit = 0;
        if (time > threshold) {
            received_bit = 1;
        } else {
            received_bit = 0;
        }

        received_value |= (received_bit << i);

        // Small busy-wait delay (no function)
        for (volatile int j = 0; j < 10000; j++);
    }

    printf("Received: %d\n", received_value);
    printf("Receiver finished.\n");

    return 0;
}

