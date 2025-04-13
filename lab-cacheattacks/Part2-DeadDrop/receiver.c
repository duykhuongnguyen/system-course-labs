// RECEIVER.C
#include "util.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE (1<<21)
#define CACHE_LINE_SIZE 64
#define BASE_SET_INDEX 500  // Same as sender
#define SET_SPACING 10

uint64_t probe_cache(volatile uint8_t *buf, int set_index) {
    uint64_t total_time = 0;
    for (int j = 0; j < 8; j++) {
        volatile uint8_t *addr = buf + ((BASE_SET_INDEX + set_index * SET_SPACING) * CACHE_LINE_SIZE) + (j * 4096);
        total_time += measure_one_block_access_time((uint64_t)addr);
    }
    return total_time / 8;
}

int main(int argc, char **argv) {
    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    if (buf == (void *) -1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    ((char *)buf)[0] = 1; // Dummy write

    printf("Please press enter.\n");
    char text_buf[2];
    fgets(text_buf, sizeof(text_buf), stdin);
    printf("Receiver now listening.\n");

    bool listening = true;

    while (listening) {
        int received = 0;

        for (int bit = 0; bit < 8; bit++) {
            uint64_t time = probe_cache((volatile uint8_t *)buf, bit);
            if (time > 200) { // Threshold: >200 cycles means "1"
                received |= (1 << bit);
            }
            usleep(500); // Sync with sender
        }

        printf("%d\n", received);
        fflush(stdout);
    }

    printf("Receiver finished.\n");
    return 0;
}
