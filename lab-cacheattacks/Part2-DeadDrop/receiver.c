
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

#define BUFF_SIZE (1<<21)

int main(int argc, char **argv)
{
    // === Put your covert channel setup code here ===
    uint8_t *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                        -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap() error\n");
        exit(1);
    }

    volatile uint8_t *target = buf + 4096 * 0; // Match sender

    printf("Please press enter.\n");

    char text_buf[2];
    fgets(text_buf, sizeof(text_buf), stdin);

    printf("Receiver now listening.\n");

    bool listening = true;
    while (listening) {

        // === Put your covert channel code here ===

        uint64_t threshold = 20; // Tune if needed
        bool start_detected = false;
        int consecutive_high = 0;

        // Step 1: Wait for start signal
        while (!start_detected) {
            uint64_t time = measure_one_block_access_time((uint64_t)target);

            if (time > threshold) {
                consecutive_high++;
            } else {
                consecutive_high = 0;
            }

            if (consecutive_high >= 5) { // Require 5 consecutive high accesses
                start_detected = true;
            }

            // Small busy-wait
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

            // Small busy-wait between bits
            for (volatile int j = 0; j < 10000; j++);
        }

        printf("Received: %d\n", received_value);

        listening = false; // End the while loop after receiving one number
    }

    printf("Receiver finished.\n");

    return 0;
}