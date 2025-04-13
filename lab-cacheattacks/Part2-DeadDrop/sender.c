// SENDER.C
#include "util.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE (1<<21)  // 2MB hugepage
#define CACHE_LINE_SIZE 64
#define BASE_SET_INDEX 500  // Arbitrary starting L2 cache set index for signaling
#define SET_SPACING 10      // Distance between sets to avoid overlap

void prime_cache(volatile uint8_t *buf, int set_index) {
    for (int j = 0; j < 8; j++) {
        volatile uint8_t *addr = buf + ((BASE_SET_INDEX + set_index * SET_SPACING) * CACHE_LINE_SIZE) + (j * 4096);
        *(volatile uint8_t *)addr;
    }
}

int main(int argc, char **argv) {
    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    if (buf == (void *) -1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    ((char *)buf)[0] = 1; // dummy write to ensure page allocation

    printf("Please type a message.\n");

    bool sending = true;
    char input_buf[16];

    while (sending) {
        fgets(input_buf, sizeof(input_buf), stdin);
        int msg = atoi(input_buf);

        for (int bit = 0; bit < 8; bit++) {
            int bit_value = (msg >> bit) & 1;
            if (bit_value == 1) {
                // Prime cache set to signal 1
                for (int k = 0; k < 1000; k++) { // Heavy access to make sure it shows
                    prime_cache((volatile uint8_t *)buf, bit);
                }
            }
            usleep(500); // Short wait between bits
        }
    }

    return 0;
}
