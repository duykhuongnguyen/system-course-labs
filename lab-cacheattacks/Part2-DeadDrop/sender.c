
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

// TODO: define your own buffer size
#define BUFF_SIZE (1<<21)
//#define BUFF_SIZE [TODO]

int main(int argc, char **argv)
{
    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);

    if (buf == (void*) -1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }

    volatile uint8_t *target = (uint8_t *)buf + 4096 * 0; // same target as receiver

    printf("Please type a message (only '0' and '1').\n");

    bool sending = true;
    while (sending) {
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        // Loop through each character the user typed
        for (int i = 0; text_buf[i] != '\0'; i++) {
            char c = text_buf[i];
            if (c == '0' || c == '1') {
                int bit = c - '0'; // convert '0'/'1' to integer

                if (bit == 1) {
                    // Send 1 by hammering memory
                    for (int j = 0; j < 1000; j++) {
                        *target;
                    }
                }
                else {
                    // Send 0 by doing nothing
                    usleep(500); // 0.5ms pause
                }
            }
        }
    }

    printf("Sender finished.\n");
    return 0;
}