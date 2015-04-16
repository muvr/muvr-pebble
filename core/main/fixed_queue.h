#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *queue_t;

///
/// Creates an empty queue
///
queue_t *queue_create(const uint8_t max_size);

///
/// Destroys an existing queue, with all its elements
///
void queue_destroy(queue_t **queue);

///
/// Adds a new element to the queue by copying ``size`` bytes from the ``buffer``.
/// After this call, ``buffer`` may be freed.
///
uint16_t queue_add(queue_t *queue, const uint32_t key, const uint8_t *buffer, const uint16_t size, const uint64_t timestamp);

///
/// Pops the first element from the queue into the ``*buffer`` with space
/// for ``size`` bytes. Returns the actual number of bytes copied, or 0
/// if the queue is empty.
///
uint16_t queue_peek(queue_t *queue, uint32_t *key, uint8_t *buffer, const uint16_t size, uint64_t *timestamp);

///
/// Mutates ``queue`` to point to its tail.
///
queue_t *queue_tail(queue_t *queue);

///
/// Returns the length of the queue
///
uint16_t queue_length(const queue_t *queue);

#ifdef __cplusplus
}
#endif
