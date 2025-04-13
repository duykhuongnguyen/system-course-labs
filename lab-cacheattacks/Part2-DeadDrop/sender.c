// sender.c
#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFF_SIZE (1 << 21) // 2MB Huge page
#define PRIMES_PER_BIT 16    // Prime more lines per bit for stronger signal

volatile uint8_t *buffer;

void prime_cache(uint8_t value) {
    // Prime the start signal
    for (int offset = 0; offset < PRIMES_PER_BIT; offset++) {
        volatile uint8_t *start_addr = buffer + offset * 64;
        *start_addr;
    }

    for (int i = 0; i < 8; i++) {
        if ((value >> i) & 1) {
            for (int offset = 0; offset < PRIMES_PER_BIT; offset++) {
                volatile uint8_t *addr = buffer + ((i + 1) * 4096) + offset * 64;
                *addr;
            }
        }
    }
}

void clear_start_signal() {
    volatile uint8_t *start_addr = buffer;
    for (int i = 0; i < 4096; i += 64) {
        *(start_addr + i);
    }
}

void cool_down() {
    for (int i = 0; i < 5000; i++) {
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
        usleep(100000); // 100ms to give receiver enough time
        clear_start_signal(); // 🛠️ Clear start signal properly
        cool_down(); // Clean up cache after sending
    }
    return 0;
}