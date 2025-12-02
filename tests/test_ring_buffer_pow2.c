#include "osusat/ring_buffer_pow2.h"
#include <assert.h>
#include <stdio.h>

static void test_init(void) {
    uint8_t storage[8];
    osusat_ring_buffer_pow2_t rb;

    // valid initialization
    assert(osusat_ring_buffer_pow2_init(&rb, storage, sizeof(storage)) == true);
    assert(rb.capacity == 8);
    assert(rb.mask == 7);
    assert(osusat_ring_buffer_pow2_empty(&rb));

    // invalid (not a power of two)
    uint8_t bad_storage[10];
    osusat_ring_buffer_pow2_t rb2;
    assert(osusat_ring_buffer_pow2_init(&rb2, bad_storage,
                                        sizeof(bad_storage)) == false);
}

static void test_push_pop(void) {
    uint8_t buf[8];
    osusat_ring_buffer_pow2_t rb;
    assert(osusat_ring_buffer_pow2_init(&rb, buf, sizeof(buf)));

    // push 3 values
    assert(osusat_ring_buffer_pow2_push(&rb, 0x11));
    assert(osusat_ring_buffer_pow2_push(&rb, 0x22));
    assert(osusat_ring_buffer_pow2_push(&rb, 0x33));

    // pop and validate
    uint8_t out;
    assert(osusat_ring_buffer_pow2_pop(&rb, &out) && out == 0x11);
    assert(osusat_ring_buffer_pow2_pop(&rb, &out) && out == 0x22);
    assert(osusat_ring_buffer_pow2_pop(&rb, &out) && out == 0x33);

    // buffer should now be empty
    assert(osusat_ring_buffer_pow2_empty(&rb));
}

static void test_full_condition(void) {
    uint8_t buf[4];
    osusat_ring_buffer_pow2_t rb;
    assert(osusat_ring_buffer_pow2_init(&rb, buf, sizeof(buf)));

    // buffer of size 4 can hold 3 bytes before full
    assert(osusat_ring_buffer_pow2_push(&rb, 1));
    assert(osusat_ring_buffer_pow2_push(&rb, 2));
    assert(osusat_ring_buffer_pow2_push(&rb, 3));

    // should now be full
    assert(osusat_ring_buffer_pow2_full(&rb) == true);

    // push should fail
    assert(osusat_ring_buffer_pow2_push(&rb, 4) == false);

    // pop one and validate
    uint8_t out;
    assert(osusat_ring_buffer_pow2_pop(&rb, &out));
    assert(out == 1);
}

int main(void) {
    test_init();
    test_push_pop();
    test_full_condition();

    printf("All pow2 ring buffer tests passed.\n");
    return 0;
}
