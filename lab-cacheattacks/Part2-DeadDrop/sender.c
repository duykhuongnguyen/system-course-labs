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
            for (int k = 0; k < 1000; k++) {
                prime_cache(buf, bit);
            }
        }
        usleep(500);
    }
}

int main(int argc, char **argv) {
    void *buf = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    if (buf == (void *) -1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    ((char *)buf)[0] = 1;

    printf("Please type a message to send:\n");

    char input_buf[256];
    while (1) {
        fgets(input_buf, sizeof(input_buf), stdin);
        size_t len = strlen(input_buf);

        // Remove trailing newline
        if (input_buf[len-1] == '\n') {
            input_buf[len-1] = '\0';
            len--;
        }

        for (size_t i = 0; i < len; i++) {
            // Double sync before every character
            send_byte((volatile uint8_t *)buf, SYNC_BYTE);
            usleep(1000);
            send_byte((volatile uint8_t *)buf, SYNC_BYTE);
            usleep(1000);

            // Send character
            send_byte((volatile uint8_t *)buf, (uint8_t)input_buf[i]);
            printf("Sent character: %c\n", input_buf[i]);
            fflush(stdout);
            usleep(5000);  // Small pause between characters
        }
    }

    return 0;
}
