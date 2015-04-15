#include "fixed_queue.h"

#if 1 == 2
#define APP_LOG_QUEUE APP_LOG_DEBUG
#else
#define APP_LOG_QUEUE(...)
#endif

queue_t *queue_create() {
    queue_t *queue = malloc(sizeof(queue_t));
    queue->length = 0;
    return queue;
}

void queue_destroy(queue_t **queue) {
    queue_t *q = *queue;
    if (q == NULL) return;

    while (q->length > 0) {
        struct queue_node *node = q->first;
        q->first = node->next;
        q->length--;
        free(node->buffer);
        free(node);
    };
    *queue = NULL;
}

struct queue_node *make_node(const uint8_t *buffer, const uint16_t size) {
    struct queue_node *node = (struct queue_node *)malloc(sizeof(struct queue_node));
    node->buffer = malloc(size);
    if (node->buffer == NULL) exit(-1); // Bantha Poodoo!

    memcpy(node->buffer, buffer, size);
    node->size = size;
    node->next = NULL;

    return node;
}

uint16_t queue_add(queue_t *queue, const uint8_t* buffer, const uint16_t size) {
    if (buffer == NULL || size == 0) return queue->length;

    struct queue_node *node = make_node(buffer, size);
    if (queue->first != NULL) {
        struct queue_node *last = queue->first;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = node;
    } else {
        queue->first = node;
    }
    queue->length++;
    return queue->length;
}

void queue_peek(queue_t *queue, uint8_t **buffer, uint16_t *size) {
    (*buffer) = queue->first->buffer;
    (*size) = queue->first->size;
}

uint16_t queue_pop(queue_t *queue, uint8_t *buffer, const uint16_t size) {
    // empty queue
    if (queue->length == 0) return 0;

    // at least one element
    struct queue_node *node = queue->first;
    if (size < node->size) return 0;

    // enough space in the buffer
    uint16_t copied_size = node->size;
    memcpy(buffer, node->buffer, copied_size);

    // drop the first element
    queue->length = queue->length - 1;
    queue->first = node->next;

    // deallocate
    free(node->buffer);
    free(node);

    return copied_size;
}

size_t queue_length(const queue_t *queue) {
    if (queue == NULL) return 0;
    
    return queue->length;
}
