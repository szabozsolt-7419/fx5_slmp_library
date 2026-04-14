/**
 * @file fx5_ringbuf.c
 * @brief Lightweight byte-oriented ring buffer implementation.
 *
 * This module provides a simple, allocation-free circular buffer
 * for byte streams. It is designed for embedded use cases where
 * predictable behavior and low overhead are required.
 *
 * Characteristics:
 * - No dynamic memory allocation
 * - Fixed-size external storage
 * - O(1) push/pop operations
 * - Partial writes supported (push_bytes)
 *
 * The buffer does not clear its underlying storage when reset or
 * initialized. Only logical state (head, tail, size) is updated.
 *
 * Typical use case:
 * - receive buffer for streaming protocols (e.g. SLMP response parsing)
 *
 * Not thread-safe.
 */
#include <fx5/fx5_ringbuf.h>


static inline uint16_t fx5_ringbuf_advance_index(uint16_t index, uint16_t capacity)
{
    index++;
    if (index >= capacity) {
        index = 0u;
    }
    return index;
}

static inline uint16_t fx5_ringbuf_offset_index(uint16_t base, uint16_t offset, uint16_t capacity)
{
    uint16_t pos = (uint16_t)(base + offset);
    if (pos >= capacity) {
        pos = (uint16_t)(pos - capacity);
    }
    return pos;
}


bool fx5_ringbuf_init(fx5_ringbuf_t *rb,
                      uint8_t *storage,
                      uint16_t capacity)
{
    if (rb == NULL) return false;
    if (storage == NULL) return false;
    if (capacity == 0u) return false;
    rb->data = storage;
    rb->capacity = capacity;
    rb->head = 0u;
    rb->tail = 0u;
    rb->size = 0u;

    return true;
}

void fx5_ringbuf_clear(fx5_ringbuf_t *rb)
{
    if (rb == NULL) return;

    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}

uint16_t fx5_ringbuf_size(const fx5_ringbuf_t *rb)
{
    return rb->size;
}

uint16_t fx5_ringbuf_capacity(const fx5_ringbuf_t *rb)
{
    return rb->capacity;
}

bool fx5_ringbuf_is_empty(const fx5_ringbuf_t *rb)
{
    return rb->size == 0;
}

bool fx5_ringbuf_is_full(const fx5_ringbuf_t *rb)
{
    return rb->size == rb->capacity;
}

bool fx5_ringbuf_push_back(fx5_ringbuf_t *rb, uint8_t value)
{
    if (rb->size == rb->capacity) {
        return false;
    }

    rb->data[rb->head] = value;

    rb->head = fx5_ringbuf_advance_index(rb->head,rb->capacity);
    rb->size++;

    return true;
}

bool fx5_ringbuf_pop_front(fx5_ringbuf_t *rb, uint8_t *out_value)
{
    if (rb->size == 0) {
        return false;
    }

    if (out_value != NULL) {
        *out_value = rb->data[rb->tail];
    }

    rb->tail = fx5_ringbuf_advance_index(rb->tail,rb->capacity);
    rb->size--;

    return true;
}

bool fx5_ringbuf_peek(const fx5_ringbuf_t *rb,
                      uint16_t index,
                      uint8_t *out_value)
{
    if (index >= rb->size) {
        return false;
    }

    uint16_t pos = fx5_ringbuf_offset_index(rb->tail,index,rb->capacity);

    if (out_value != NULL) {
        *out_value = rb->data[pos];
    }

    return true;
}

uint16_t fx5_ringbuf_push_bytes(fx5_ringbuf_t *rb,
                               const uint8_t *data,
                               uint16_t length)
{
    uint16_t written = 0;

    while (written < length) {
        if (!fx5_ringbuf_push_back(rb, data[written])) {
            break;
        }
        written++;
    }

    return written;
}

uint16_t fx5_ringbuf_drop_front(fx5_ringbuf_t *rb,
                               uint16_t count)
{
    uint16_t dropped = 0;

    while (dropped < count && rb->size > 0) {
        rb->tail = fx5_ringbuf_advance_index(rb->tail,rb->capacity);
        rb->size--;
        dropped++;
    }

    return dropped;
}