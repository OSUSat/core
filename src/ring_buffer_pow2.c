#include "osusat/ring_buffer_pow2.h"

bool osusat_ring_buffer_pow2_init(osusat_ring_buffer_pow2_t *rb,
                                  uint8_t *storage, size_t capacity) {
    if (!osusat_is_pow2(capacity)) {
        return false;
    }

    rb->buffer = storage;
    rb->capacity = capacity;
    rb->mask = capacity - 1;
    rb->head = 0;
    rb->tail = 0;

    return true;
}

bool osusat_ring_buffer_pow2_push(osusat_ring_buffer_pow2_t *rb, uint8_t byte) {
    size_t next = (rb->head + 1) & rb->mask;

    if (next == rb->tail) {
        return false; // full
    }

    rb->buffer[rb->head] = byte;
    rb->head = next;

    return true;
}

bool osusat_ring_buffer_pow2_pop(osusat_ring_buffer_pow2_t *rb, uint8_t *out) {
    if (rb->head == rb->tail) {
        return false; // empty
    }

    *out = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & rb->mask;

    return true;
}

bool osusat_ring_buffer_pow2_peek(const osusat_ring_buffer_pow2_t *rb,
                                  uint8_t *out) {
    if (rb->head == rb->tail) {
        return false;
    }

    *out = rb->buffer[rb->tail];

    return true;
}
