// sender.c
#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFF_SIZE (1 << 21) // 2MB Huge page

volatile uint8_t *buffer;

void prime_cache(uint8_t value) {
    // Prime the start signal
    volatile uint8_t *start_addr = buffer;
    *start_addr;

    for (int i = 0; i < 8; i++) {
        if ((value >> i) & 1) {
            volatile uint8_t *addr = buffer + ((i + 1) * 64);
            *addr;
        }
    }
}

void cool_down() {
    for (int i = 0; i < 1000; i++) {
        volatile uint8_t *addr = buffer + (rand() % BUFF_SIZE);
        *addr;
    }
}

int main() {
    buffer = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    printf("Please type a message.\n");
    while (1) {
        int value;
        scanf("%d", &value);
        prime_cache((uint8_t)value);
        usleep(50000); // 50ms delay for receiver to catch up
        cool_down(); // Clean up cache after sending
    }
    return 0;
}