#include "osusat/ring_buffer.h"
#include <assert.h>

void test_basic(void) {
    uint8_t buf[4];
    osusat_ring_buffer_t rb;

    osusat_ring_buffer_init(&rb, buf, sizeof(buf), false);

    assert(osusat_ring_buffer_empty(&rb));

    osusat_ring_buffer_push(&rb, 1);
    osusat_ring_buffer_push(&rb, 2);

    uint8_t out;
    assert(osusat_ring_buffer_pop(&rb, &out) && out == 1);
    assert(osusat_ring_buffer_pop(&rb, &out) && out == 2);
}

int main(void) {
    test_basic();
    return 0;
}
