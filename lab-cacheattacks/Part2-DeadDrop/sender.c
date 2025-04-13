// SENDER.C (Send full string)
#include "util.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE (1<<21)
#define CACHE_LINE_SIZE 64
#define BASE_SET_INDEX 500
#define SET_SPACING 10
#define SYNC_BYTE 170

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
            for (int k = 0; k < 5000; k++) {  // INCREASED hammering!
                prime_cache(buf, bit);
            }
        }
        usleep(1000); // INCREASED sleep between bits
    }
}

int main(int argc, char **argv) {
    ...
    printf("Please type a message to send:\n");

    char input_buf[256];
    while (1) {
        fgets(input_buf, sizeof(input_buf), stdin);
        size_t len = strlen(input_buf);

        if (input_buf[len-1] == '\n') {
            input_buf[len-1] = '\0';
            len--;
        }

        for (size_t i = 0; i < len; i++) {
            // DOUBLE SYNC
            send_byte((volatile uint8_t *)buf, 170);
            usleep(5000);  // INCREASED sleep
            send_byte((volatile uint8_t *)buf, 170);
            usleep(5000);

            // SEND CHARACTER
            send_byte((volatile uint8_t *)buf, (uint8_t)input_buf[i]);
            printf("Sent character: %c\n", input_buf[i]);
            fflush(stdout);

            usleep(10000); // BIGGER sleep between characters
        }
    }
}