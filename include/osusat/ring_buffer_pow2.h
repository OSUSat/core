/**
 * @file osusat_ring_buffer_pow2.h
 * @brief High-performance Power-of-Two Ring Buffer.
 *
 * A specialized circular buffer implementation that relies on the
 * storage capacity being a power of two (2, 4, 8, ...).
 *
 * Key Differences from Generic implementation:
 * - Uses bitwise AND instead of modulo for index wrapping.
 * - Significantly faster on microcontrollers lacking hardware division.
 * - Strict initialization requirement: capacity MUST be a power of two.
 */

#ifndef OSUSAT_RING_BUFFER_POW2_H
#define OSUSAT_RING_BUFFER_POW2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup osusat_ring_buffer_pow2 Fast Ring Buffer
 * @brief Bitmask-optimized circular buffer.
 *
 * @{
 */

/**
 * @defgroup osusat_ring_buffer_pow2_types Structures
 * @ingroup osusat_ring_buffer_pow2
 * @brief Structures used by the Fast Ring Buffer.
 *
 * @{
 */

/**
 * @struct osusat_ring_buffer_pow2_t
 * @brief Fast ring buffer state structure.
 */
typedef struct {
    uint8_t *buffer; /**< Pointer to the underlying storage array */
    size_t capacity; /**< Total size (MUST be power of two) */
    size_t mask; /**< Pre-calculated mask (capacity - 1) for fast wrapping */
    size_t head; /**< Write index */
    size_t tail; /**< Read index */
} osusat_ring_buffer_pow2_t;

/** @} */ // end osusat_ring_buffer_pow2_types

/**
 * @defgroup osusat_ring_buffer_pow2_api Public API
 * @ingroup osusat_ring_buffer_pow2
 * @brief External interface for the Fast Ring Buffer.
 * @{
 */

/**
 * @brief Utility to check if a number is a power of two.
 *
 * @param[in] x The number to check.
 * @return true if x is a power of two, false otherwise.
 */
static inline bool osusat_is_pow2(size_t x) { return x && !(x & (x - 1)); }

/**
 * @brief Initialize the optimized ring buffer.
 *
 * Calculates the bitmask based on the provided capacity.
 *
 * @warning The `capacity` MUST be a power of two (e.g., 32, 64, 128).
 * If a non-power-of-two is provided, initialization will fail.
 *
 * @param[out] rb       Pointer to the ring buffer handle.
 * @param[in]  storage  Pointer to the allocated byte array.
 * @param[in]  capacity Size of storage in bytes.
 *
 * @retval true  Initialization successful.
 * @retval false Initialization failed (capacity was not a power of two).
 */
bool osusat_ring_buffer_pow2_init(osusat_ring_buffer_pow2_t *rb,
                                  uint8_t *storage, size_t capacity);

/**
 * @brief Reset the buffer to empty.
 *
 * Resets head and tail indices to zero.
 *
 * @param[in,out] rb The ring buffer handle.
 */
static inline void
osusat_ring_buffer_pow2_clear(osusat_ring_buffer_pow2_t *rb) {
    rb->head = rb->tail = 0;
}

/**
 * @brief Check if the buffer is empty.
 *
 * @param[in] rb The ring buffer handle.
 * @retval true  Buffer is empty.
 * @retval false Buffer contains data.
 */
static inline bool
osusat_ring_buffer_pow2_empty(const osusat_ring_buffer_pow2_t *rb) {
    return rb->head == rb->tail;
}

/**
 * @brief Check if the buffer is full.
 *
 * Uses bitwise masking to check the next write position.
 *
 * @param[in] rb The ring buffer handle.
 * @retval true  Buffer is full.
 * @retval false Buffer has space available.
 */
static inline bool
osusat_ring_buffer_pow2_full(const osusat_ring_buffer_pow2_t *rb) {
    return ((rb->head + 1) & rb->mask) == rb->tail;
}

/**
 * @brief Push one byte into the buffer.
 *
 * @param[in,out] rb   The ring buffer handle.
 * @param[in]     byte The byte to store.
 *
 * @retval true  Success, byte added.
 * @retval false Buffer was full.
 */
bool osusat_ring_buffer_pow2_push(osusat_ring_buffer_pow2_t *rb, uint8_t byte);

/**
 * @brief Pop one byte from the buffer.
 *
 * @param[in,out] rb  The ring buffer handle.
 * @param[out]    out Pointer to where the popped byte will be written.
 *
 * @retval true  Success, byte written to `out`.
 * @retval false Buffer was empty.
 */
bool osusat_ring_buffer_pow2_pop(osusat_ring_buffer_pow2_t *rb, uint8_t *out);

/**
 * @brief Peek at the next byte without removing it.
 *
 * @param[in]  rb  The ring buffer handle.
 * @param[out] out Pointer to where the peeked byte will be written.
 *
 * @retval true  Success, byte written to `out`.
 * @retval false Buffer was empty.
 */
bool osusat_ring_buffer_pow2_peek(const osusat_ring_buffer_pow2_t *rb,
                                  uint8_t *out);

/** @} */ // end osusat_ring_buffer_pow2_api

/** @} */ // end osusat_ring_buffer_pow2

#ifdef __cplusplus
}
#endif

#endif // OSUSAT_RING_BUFFER_POW2_H
