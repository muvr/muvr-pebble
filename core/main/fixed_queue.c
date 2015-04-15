#include "fixed_queue.h"
#include "compat.h"
#include <pebble.h>

struct internal_queue_node_t {
    uint8_t *buffer;
    uint16_t size;
    struct internal_queue_node_t *next;
};

struct internal_queue_t {
    uint16_t length;
    struct internal_queue_node_t *first;
};


queue_t *queue_create() {
    struct internal_queue_t *queue = malloc(sizeof(queue_t));
    queue->length = 0;
    queue->first = NULL;
    return (queue_t *)queue;
}

void queue_destroy(queue_t **queue) {
    struct internal_queue_t *q = (struct internal_queue_t *) *queue;
    if (q == NULL) return;

    while (q->first != NULL) {
        struct internal_queue_node_t *node = q->first;
        q->first = node->next;
        free(node->buffer);
        free(node);
    }

    *queue = NULL;
}

struct internal_queue_node_t *make_node(const uint8_t *buffer, const uint16_t size) {
    struct internal_queue_node_t *node = (struct internal_queue_node_t *)malloc(sizeof(struct internal_queue_node_t));
    node->buffer = malloc(size);
    if (node->buffer == NULL) EXIT(-1); // Bantha Poodoo!

    memcpy(node->buffer, buffer, size);
    node->size = size;
    node->next = NULL;

    return node;
}

uint16_t queue_add(queue_t *queue, const uint8_t* buffer, const uint16_t size) {
    struct internal_queue_t *q = (struct internal_queue_t *) queue;
    if (buffer == NULL || size == 0) return q->length;

    struct internal_queue_node_t *node = make_node(buffer, size);
    if (q->first != NULL) {
        struct internal_queue_node_t *last = q->first;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = node;
    } else {
        q->first = node;
    }
    q->length++;
    return q->length;
}

uint16_t queue_peek(queue_t *queue, uint8_t *buffer, const uint16_t size) {
    struct internal_queue_t *q = (struct internal_queue_t *) queue;
    // empty queue
    if (q->length == 0) return 0;

    // at least one element
    struct internal_queue_node_t *node = q->first;
    if (size < node->size) return node->size;

    // enough space in the buffer
    uint16_t copied_size = node->size;
    memcpy(buffer, node->buffer, copied_size);

    return copied_size;
}

queue_t *queue_tail(queue_t *queue) {
    struct internal_queue_t *q = (struct internal_queue_t *) queue;
    // empty
    if (q->length == 0) return (queue_t *) q;

    // at least one element
    struct internal_queue_node_t *node = q->first;

    // unlink the node
    q->first = node->next;
    q->length--;

    // deallocate its memory
    free(node->buffer);
    free(node);

    // return mutated queue
    return queue;
}

uint16_t queue_length(const queue_t *queue) {
    if (queue == NULL) return 0;
    struct internal_queue_t *q = (struct internal_queue_t *) queue;

    return q->length;
}
