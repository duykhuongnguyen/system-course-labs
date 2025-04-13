// sender.c
#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFF_SIZE (1 << 21) // 2MB Huge page
#define START_SIGNAL_LINE 100 // Use cache line 100 for start signal

volatile uint8_t *buffer;

void prime_cache(uint8_t value) {
    // Prime the start signal (touch buffer[4096 * 100])
    volatile uint8_t *start_addr = buffer + (START_SIGNAL_LINE * 4096);
    *start_addr;

    for (int i = 0; i < 8; i++) {
        if ((value >> i) & 1) {
            volatile uint8_t *addr = buffer + ((i + 1) * 4096);
            *addr;
        }
    }
}

void clear_start_signal() {
    volatile uint8_t *start_addr = buffer + (START_SIGNAL_LINE * 4096);
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
        printf("[Sender] Sent value: %d\n", value);
        clear_start_signal();
        cool_down();
    }
    return 0;
}
