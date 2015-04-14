#include "fixed_queue.h"

#if 1 == 2
#define APP_LOG_QUEUE APP_LOG_DEBUG
#else
#define APP_LOG_QUEUE(...)
#endif

//
static queue_t queue;

queue_t *queue_create() {
    queue.length = 0;
    return &queue;
}

void queue_destroy(queue_t **queue) {
    APP_LOG_QUEUE("queue_destroy - begin");

    queue_t * pQueue = (*queue);
    if (pQueue == NULL) {
        APP_LOG_QUEUE("queue_destroy - end");
        return;
    }

    (*queue) = NULL;
    while (pQueue->length > 0) {
        uint8_t* buffer;
        uint16_t size;

        queue_pop(pQueue, &buffer, &size);
        if (buffer != NULL) free(buffer);
    };

    free(pQueue);

    APP_LOG_QUEUE("queue_destroy - end");
}

void queue_add(queue_t *queue, uint8_t* buffer, uint16_t size) {
    if (queue->length > 0) {
        struct queue_node *last = (struct queue_node *)malloc(sizeof(struct queue_node));
        void *ptr = malloc(size);
        memcpy(ptr, buffer, size);
        last->buffer = (uint8_t *)ptr;
        last->size = size;
        last->next = NULL;

        struct queue_node * current = queue->first;
        while (current->next != NULL) current = current->next;
        current->next = last;
        queue->length = queue->length + 1;
    } else {
        struct queue_node *last = (struct queue_node *)malloc(sizeof(struct queue_node));
        void *ptr = malloc(size);
        memcpy(ptr, buffer, size);
        last->buffer = (uint8_t *)ptr;
        last->size = size;
        last->next = NULL;
        queue->length = 1;
        queue->first = last;
    }

    APP_LOG_QUEUE("queue_add - end");
}

void queue_peek(queue_t *queue, uint8_t **buffer, uint16_t *size) {
    APP_LOG_QUEUE("queue_peek - begin");

    (*buffer) = queue->first->buffer;
    (*size) = queue->first->size;

    APP_LOG_QUEUE("queue_peek - end");
}

void queue_pop(queue_t *queue, uint8_t **buffer, uint16_t *size) {
    APP_LOG_QUEUE("queue_pop - begin");

    struct queue_node *node = queue->first;
    (*buffer) = node->buffer;
    (*size) = node->size;
    
    if (node->next == NULL) {
        node->buffer = NULL;
        node->size = 0;
        queue->length = 0;
    } else {
        queue->length = queue->length - 1;
        queue->first = node->next;
        free(node);
    }
    
    APP_LOG_QUEUE("queue_pop - end");
}

size_t queue_length(queue_t *queue) {
    if (queue == NULL) return 0;
    
    return queue->length;
}
