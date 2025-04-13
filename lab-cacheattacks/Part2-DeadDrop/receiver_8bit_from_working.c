#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define BIT_DURATION_US 200
#define THRESHOLD 150

static uint8_t probe_array[64 * 300];

int main() {
    printf("Press enter to start receiving...\n");
    getchar();
    printf("Receiver listening.\n");

    while (1) {
        int result = 0;
        for (int i = 0; i < 8; i++) {
            uint64_t total = 0;
            for (int j = 0; j < 300; j++) {
                total += measure_one_block_access_time((uint64_t)&probe_array[j * 64]);
            }
            int avg = total / 300;
            int bit = (avg > THRESHOLD) ? 1 : 0;

            printf("Bit %d: avg = %d → %d\n", i, avg, bit);
            fflush(stdout);

            result |= (bit << i);
            usleep(BIT_DURATION_US);
        }

        printf("Received integer: %d\n", result);
        fflush(stdout);
    }

    return 0;
}
