#include "osusat/ring_buffer.h"

void osusat_ring_buffer_init(osusat_ring_buffer_t *rb, uint8_t *storage,
                             size_t capacity, bool overwrite) {
    rb->buffer = storage;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->overwrite = overwrite;
}

void osusat_ring_buffer_clear(osusat_ring_buffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

bool osusat_ring_buffer_full(const osusat_ring_buffer_t *rb) {
    return ((rb->head + 1) % rb->capacity) == rb->tail;
}

size_t osusat_ring_buffer_size(const osusat_ring_buffer_t *rb) {
    if (rb->head >= rb->tail)
        return rb->head - rb->tail;

    return rb->capacity - (rb->tail - rb->head);
}

bool osusat_ring_buffer_push(osusat_ring_buffer_t *rb, uint8_t byte) {
    size_t next = (rb->head + 1) % rb->capacity;

    if (next == rb->tail) {
        // buffer full
        if (!rb->overwrite) {
            return false;
        }

        // overwrite oldest: move tail forward
        rb->tail = (rb->tail + 1) % rb->capacity;
    }

    rb->buffer[rb->head] = byte;
    rb->head = next;

    return true;
}

bool osusat_ring_buffer_pop(osusat_ring_buffer_t *rb, uint8_t *out) {
    if (rb->tail == rb->head) {
        return false; // empty
    }

    *out = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;

    return true;
}

bool osusat_ring_buffer_peek(const osusat_ring_buffer_t *rb, uint8_t *out) {
    if (rb->tail == rb->head) {
        return false;
    }

    *out = rb->buffer[rb->tail];
    return true;
}
