#include "osusat/ring_buffer.h"
#include <stdio.h>

int main(void) {
    uint8_t storage[16];

    osusat_ring_buffer_t rb;
    osusat_ring_buffer_init(&rb, storage, sizeof(storage), false);

    osusat_ring_buffer_push(&rb, 0x42);
    osusat_ring_buffer_push(&rb, 0x43);

    uint8_t v;
    while (osusat_ring_buffer_pop(&rb, &v)) {
        printf("Popped: %02X\n", v);
    }

    return 0;
}
