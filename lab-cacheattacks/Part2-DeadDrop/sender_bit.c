#include "util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define BIT_DURATION_US 200

static uint8_t prime_array[64 * 300];

int main() {
    printf("Enter 0 or 1:\n");

    while (1) {
        char line[8];
        fgets(line, sizeof(line), stdin);
        int bit = atoi(line);

        if (bit == 1) {
            for (int i = 0; i < 300; i++) {
                volatile uint8_t tmp = prime_array[i * 64];
            }
        } else {
            usleep(BIT_DURATION_US);
        }

        usleep(BIT_DURATION_US);  // spacing
    }

    return 0;
}
