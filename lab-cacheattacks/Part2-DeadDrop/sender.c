
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

// TODO: define your own buffer size
#define BUFF_SIZE (1<<21)
//#define BUFF_SIZE [TODO]

int main(int argc, char **argv)
{
    // Allocate buffer using huge page
    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);
    if (buf == (void*) -1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }

    volatile uint8_t *target = (uint8_t *)buf + 4096 * 0; // Same target as receiver

    printf("Please type a number (0-255).\n");

    bool sending = true;
    while (sending) {
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        int value = atoi(text_buf); // Convert input string to integer (0-255)

        // Step 1: Send a start signal
        for (int j = 0; j < 1000; j++) {
            *target;
        }

        // Small busy-wait after start signal
        for (volatile int i = 0; i < 10000; i++);

        // Step 2: Send 8 bits
        for (int i = 7; i >= 0; i--) {
            int bit = (value >> i) & 1;

            if (bit == 1) {
                for (int j = 0; j < 1000; j++) {
                    *target;
                }
            } else {
                // Small idle time for 0 bit
                for (volatile int j = 0; j < 10000; j++);
            }

            // Small pause between bits
            for (volatile int j = 0; j < 10000; j++);
        }
    }

    printf("Sender finished.\n");
    return 0;
}