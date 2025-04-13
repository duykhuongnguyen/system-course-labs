#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define BIT_DURATION_US 200

static uint8_t prime_array[64 * 300];

int main() {
    printf("Enter integer 0–255:\n");

    while (1) {
        char line[16];
        fgets(line, sizeof(line), stdin);
        int val = atoi(line);

        if (val < 0 || val > 255) {
            printf("Invalid input.\n");
            continue;
        }

        for (int i = 0; i < 8; i++) {
            int bit = (val >> i) & 1;
            printf("Sending bit %d: %d\n", i, bit);
            fflush(stdout);

            if (bit == 1) {
                for (int j = 0; j < 300; j++) {
                    volatile uint8_t tmp = prime_array[j * 64];
                }
            } else {
                usleep(BIT_DURATION_US);
            }

            usleep(BIT_DURATION_US);
        }
    }

    return 0;
}
