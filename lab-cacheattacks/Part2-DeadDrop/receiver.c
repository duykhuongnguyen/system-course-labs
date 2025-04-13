// receiver.c
#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFF_SIZE (1 << 21)
#define THRESHOLD 150

volatile uint8_t *buffer;

int probe_cache_line(int idx) {
    volatile uint8_t *addr = buffer + (idx * 64);
    uint64_t time = measure_one_block_access_time((void *)addr);
    return time;
}

void wait_for_start_signal() {
    while (1) {
        int access_time = probe_cache_line(0); // Line 0 is start signal
        if (access_time > THRESHOLD) {
            break;
        }
        usleep(1000); // Sleep 1ms between checks
    }
}

int main() {
    buffer = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    printf("Please press enter.\n");
    getchar();
    printf("Receiver now listening.\n");

    while (1) {
        wait_for_start_signal();
        uint8_t received = 0;
        for (int i = 0; i < 8; i++) {
            int access_time = probe_cache_line(i + 1);
            if (access_time > THRESHOLD) {
                received |= (1 << i);
            }
        }
        printf("%d\n", received);
        fflush(stdout);
        usleep(50000); // Wait a bit before next listen
    }
    return 0;
}
