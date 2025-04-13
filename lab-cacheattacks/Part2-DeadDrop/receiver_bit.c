#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define BIT_DURATION_US 200
#define THRESHOLD 150  // empirically tune this!

int main() {
    static uint8_t probe_array[64 * 300] = {0};

    printf("Please press enter to start receiving...\n");
    getchar();
    printf("Receiver now listening.\n");

    while (1) {
        uint64_t total = 0;
        for (int i = 0; i < 300; i++) {
            total += measure_one_block_access_time((uint64_t)&probe_array[i * 64]);
        }

        int avg = total / 300;
        int bit = (avg > THRESHOLD) ? 1 : 0;

        printf("Received bit: %d (avg: %d)\n", bit, avg);
        fflush(stdout);
        usleep(BIT_DURATION_US);
    }

    return 0;
}
