/**
 * @file osusat_ring_buffer.h
 * @brief OSUSat Embedded Ring Buffer implementation.
 *
 * A generic, modulo-based circular buffer designed for usage in OSUSat
 * firmware.
 *
 * Features:
 * - Fixed-size circular buffer (using external storage)
 * - Single-producer/single-consumer friendly (ISR safe without locks in this
 * specific context)
 * - O(1) complexity for all operations
 * - Supports arbitrary sizes (does not require power-of-two)
 * - Optional overwrite mode for "newest data only" logging
 */

#ifndef OSUSAT_RING_BUFFER_H
#define OSUSAT_RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup osusat_ring_buffer Ring Buffer
 * @brief Generic byte-oriented circular buffer.
 *
 * @{
 */

/**
 * @defgroup osusat_ring_buffer_types Structures
 * @ingroup osusat_ring_buffer
 * @brief Structures used by the Ring Buffer.
 *
 * @{
 */

/**
 * @struct osusat_ring_buffer_t
 * @brief Ring buffer state structure.
 *
 * @note This structure should be initialized via ::osusat_ring_buffer_init
 * before use.
 */
typedef struct {
    uint8_t *buffer; /**< Pointer to the underlying storage array */
    size_t capacity; /**< Total size of the storage array */
    size_t head;     /**< Write index */
    size_t tail;     /**< Read index */
    bool overwrite; /**< If true: pushing to a full buffer overwrites the oldest
                      byte */
} osusat_ring_buffer_t;

/** @} */ // end osusat_ring_buffer_types

/**
 * @defgroup osusat_ring_buffer_api Public API
 * @ingroup osusat_ring_buffer
 * @brief External interface for interacting with the Ring Buffer.
 *
 * @{
 */

/**
 * @brief Initialize a ring buffer.
 *
 * Sets up the control structure to point to the provided storage array.
 *
 * @param[out] rb        Pointer to the ring buffer handle to initialize.
 * @param[in]  storage   Pointer to the allocated byte array to use as storage.
 * @param[in]  capacity  Size of the storage array in bytes.
 * @param[in]  overwrite If true, the buffer will overwrite the oldest data
 * when full. If false, it will reject new data when full.
 */
void osusat_ring_buffer_init(osusat_ring_buffer_t *rb, uint8_t *storage,
                             size_t capacity, bool overwrite);

/**
 * @brief Reset the buffer to empty.
 *
 * Resets head and tail indices to zero. Does not zero-out the underlying
 * memory.
 *
 * @param[in,out] rb The ring buffer handle.
 */
void osusat_ring_buffer_clear(osusat_ring_buffer_t *rb);

/**
 * @brief Push one byte into the buffer.
 *
 * @param[in,out] rb   The ring buffer handle.
 * @param[in]     byte The byte to store.
 *
 * @retval true  Successfully added (or overwrote) the byte.
 * @retval false Buffer was full and overwrite mode is disabled.
 */
bool osusat_ring_buffer_push(osusat_ring_buffer_t *rb, uint8_t byte);

/**
 * @brief Pop one byte from the buffer.
 *
 * @param[in,out] rb  The ring buffer handle.
 * @param[out]    out Pointer to where the popped byte will be written.
 *
 * @retval true  Success, byte written to `out`.
 * @retval false Buffer was empty.
 */
bool osusat_ring_buffer_pop(osusat_ring_buffer_t *rb, uint8_t *out);

/**
 * @brief Peek at the next byte without removing it.
 *
 * Useful for inspecting the next available data packet header without
 * consuming it.
 *
 * @param[in]  rb  The ring buffer handle.
 * @param[out] out Pointer to where the peeked byte will be written.
 *
 * @retval true  Success, byte written to `out`.
 * @retval false Buffer was empty.
 */
bool osusat_ring_buffer_peek(const osusat_ring_buffer_t *rb, uint8_t *out);

/**
 * @brief Get the number of bytes currently stored.
 *
 * @param[in] rb The ring buffer handle.
 * @return Number of bytes active in the buffer.
 */
size_t osusat_ring_buffer_size(const osusat_ring_buffer_t *rb);

/**
 * @brief Check if the buffer is empty.
 *
 * @param[in] rb The ring buffer handle.
 * @retval true  Buffer is empty.
 * @retval false Buffer contains data.
 */
static inline bool osusat_ring_buffer_empty(const osusat_ring_buffer_t *rb) {
    return rb->head == rb->tail;
}

/**
 * @brief Check if the buffer is full.
 *
 * @param[in] rb The ring buffer handle.
 * @retval true  Buffer is full.
 * @retval false Buffer has space available.
 */
bool osusat_ring_buffer_full(const osusat_ring_buffer_t *rb);

/** @} */ // end osusat_ring_buffer_api

/** @} */ // end osusat_ring_buffer

#ifdef __cplusplus
}
#endif

#endif // OSUSAT_RING_BUFFER_H
