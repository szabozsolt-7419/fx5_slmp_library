#ifndef FX5_RINGBUF_H
#define FX5_RINGBUF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file fx5_ringbuf.h
 * @brief Lightweight byte-oriented ring buffer API.
 *
 * This module provides a small allocation-free circular buffer for byte streams.
 * Storage is supplied by the caller. The buffer maintains only logical state
 * (head, tail, size); it does not clear the underlying storage when initialized
 * or reset.
 *
 * Unless stated otherwise, functions other than fx5_ringbuf_init() expect a
 * previously initialized ring buffer.
 */

/**
 * @brief Byte-oriented ring buffer state.
 */
typedef struct
{
    uint8_t *data;     /**< External storage buffer. */
    uint16_t capacity; /**< Storage capacity in bytes. */

    uint16_t head;     /**< Write position. */
    uint16_t tail;     /**< Read position. */
    uint16_t size;     /**< Number of stored bytes. */
} fx5_ringbuf_t;

/**
 * @brief Initialize a ring buffer with caller-provided storage.
 *
 * @param[out] rb Ring buffer object to initialize.
 * @param[in] storage Backing storage for the buffer.
 * @param[in] capacity Storage capacity in bytes. Must be greater than zero.
 * @return true on success, false if any argument is invalid.
 */
bool fx5_ringbuf_init(fx5_ringbuf_t *rb,
                      uint8_t *storage,
                      uint16_t capacity);

/**
 * @brief Reset the logical contents of a ring buffer.
 *
 * The underlying storage bytes are not cleared.
 *
 * @param[in,out] rb Ring buffer to clear. If NULL, the function has no effect.
 */
void fx5_ringbuf_clear(fx5_ringbuf_t *rb);

/**
 * @brief Return the number of stored bytes.
 *
 * @param[in] rb Initialized ring buffer.
 * @return Current number of stored bytes.
 */
uint16_t fx5_ringbuf_size(const fx5_ringbuf_t *rb);

/**
 * @brief Return the storage capacity in bytes.
 *
 * @param[in] rb Initialized ring buffer.
 * @return Buffer capacity in bytes.
 */
uint16_t fx5_ringbuf_capacity(const fx5_ringbuf_t *rb);

/**
 * @brief Check whether the buffer is empty.
 *
 * @param[in] rb Initialized ring buffer.
 * @return true if empty, otherwise false.
 */
bool fx5_ringbuf_is_empty(const fx5_ringbuf_t *rb);

/**
 * @brief Check whether the buffer is full.
 *
 * @param[in] rb Initialized ring buffer.
 * @return true if full, otherwise false.
 */
bool fx5_ringbuf_is_full(const fx5_ringbuf_t *rb);

/**
 * @brief Append one byte to the buffer.
 *
 * @param[in,out] rb Initialized ring buffer.
 * @param[in] value Byte to append.
 * @return true on success, false if the buffer is full.
 */
bool fx5_ringbuf_push_back(fx5_ringbuf_t *rb, uint8_t value);

/**
 * @brief Remove one byte from the front of the buffer.
 *
 * @param[in,out] rb Initialized ring buffer.
 * @param[out] out_value Optional output pointer for the removed byte.
 * @return true on success, false if the buffer is empty.
 */
bool fx5_ringbuf_pop_front(fx5_ringbuf_t *rb, uint8_t *out_value);

/**
 * @brief Read a byte at a logical index without removing it.
 *
 * Index 0 refers to the oldest byte currently stored in the buffer.
 *
 * @param[in] rb Initialized ring buffer.
 * @param[in] index Logical index from the front of the buffer.
 * @param[out] out_value Optional output pointer for the read byte.
 * @return true on success, false if @p index is out of range.
 */
bool fx5_ringbuf_peek(const fx5_ringbuf_t *rb,
                      uint16_t index,
                      uint8_t *out_value);

/**
 * @brief Append as many bytes as possible from an input array.
 *
 * @param[in,out] rb Initialized ring buffer.
 * @param[in] data Input byte array.
 * @param[in] length Number of bytes to append.
 * @return Number of bytes actually written.
 */
uint16_t fx5_ringbuf_push_bytes(fx5_ringbuf_t *rb,
                                const uint8_t *data,
                                uint16_t length);

/**
 * @brief Drop up to @p count bytes from the front of the buffer.
 *
 * @param[in,out] rb Initialized ring buffer.
 * @param[in] count Maximum number of bytes to remove.
 * @return Number of bytes actually removed.
 */
uint16_t fx5_ringbuf_drop_front(fx5_ringbuf_t *rb,
                                uint16_t count);

#ifdef __cplusplus
}
#endif

#endif /* FX5_RINGBUF_H */