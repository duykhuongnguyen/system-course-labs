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
#define BASE_SET_INDEX 500
#define SET_SPACING 10

void prime_cache(volatile uint8_t *buf, int set_index) {
    for (int j = 0; j < 8; j++) {
        volatile uint8_t *addr = buf + ((BASE_SET_INDEX + set_index * SET_SPACING) * CACHE_LINE_SIZE) + (j * 4096);
        *(volatile uint8_t *)addr;
    }
}

void send_byte(volatile uint8_t *buf, uint8_t byte) {
    for (int bit = 0; bit < 8; bit++) {
        int bit_value = (byte >> bit) & 1;
        if (bit_value == 1) {
            for (int k = 0; k < 1000; k++) {
                prime_cache(buf, bit);
            }
        }
        usleep(500); // short wait between bits
    }
}

int main(int argc, char **argv) {
    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    if (buf == (void *) -1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    ((char *)buf)[0] = 1; // Dummy write

    printf("Please type a message (integer 0-255):\n");

    char input_buf[16];
    while (1) {
        fgets(input_buf, sizeof(input_buf), stdin);
        int msg = atoi(input_buf);

        // Step 1: Send synchronization signal
        send_byte((volatile uint8_t *)buf, 0xFF);  // 255 indicates start

        usleep(1000); // Little extra gap

        // Step 2: Send actual message
        send_byte((volatile uint8_t *)buf, (uint8_t)msg);

        printf("Sent: %d\n", msg);
        fflush(stdout);
    }

    return 0;
}
