#include "osusat/ring_buffer_pow2.h"
#include <stdint.h>
#include <stdio.h>

int main(void) {
    uint8_t storage[16]; // must be power-of-two
    osusat_ring_buffer_pow2_t rb;

    if (!osusat_ring_buffer_pow2_init(&rb, storage, sizeof(storage))) {
        printf("Failed to initialize pow2 ring buffer (capacity must be a "
               "power of 2)\n");
        return 1;
    }

    // push some bytes
    osusat_ring_buffer_pow2_push(&rb, 0x41);
    osusat_ring_buffer_pow2_push(&rb, 0x42);
    osusat_ring_buffer_pow2_push(&rb, 0x43);

    // pop and print all bytes
    uint8_t out;

    while (osusat_ring_buffer_pow2_pop(&rb, &out)) {
        printf("Popped: %02X\n", out);
    }

    return 0;
}
