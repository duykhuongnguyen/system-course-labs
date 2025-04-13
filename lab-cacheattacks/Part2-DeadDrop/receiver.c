// receiver.c
#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BUFF_SIZE (1 << 21)
#define THRESHOLD 160
#define NUM_SAMPLES 5

volatile uint8_t *buffer;

int probe_cache_line(int idx) {
    volatile uint8_t *addr = buffer + (idx * 4096);
    uint64_t time = measure_one_block_access_time((void *)addr);
    return time;
}

int read_bit(int idx) {
    int total_time = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        total_time += probe_cache_line(idx);
    }
    int avg_time = total_time / NUM_SAMPLES;
    return (avg_time > THRESHOLD) ? 1 : 0;
}

void wait_for_start_signal() {
    // First wait until start signal is CLEAR (low latency)
    while (1) {
        int access_time = probe_cache_line(0);
        if (access_time < THRESHOLD) {
            break;
        }
        usleep(5000);
    }

    // Then wait until start signal is SET (high latency)
    while (1) {
        int access_time = probe_cache_line(0);
        if (access_time > THRESHOLD) {
            break;
        }
        usleep(5000);
    }
}

int main() {
    buffer = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
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
            if (read_bit(i + 1)) {
                received |= (1 << i);
            }
        }
        printf("%d\n", received);
        fflush(stdout);
        usleep(100000);
    }
    return 0;
}