#pragma once

#include "pebble.h"
#include "debug_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct queue_node {
    uint8_t *buffer;
    uint16_t size;
    struct queue_node *next;
};

typedef struct {
    size_t length;
    struct queue_node *first;
} queue_t;
    
queue_t *queue_create();
void queue_destroy(queue_t **queue);
void queue_add(queue_t *queue, uint8_t *buffer, uint16_t size);
void queue_peek(queue_t *queue, uint8_t **buffer, uint16_t *size);
void queue_pop(queue_t *queue, uint8_t **buffer, uint16_t *size);
size_t queue_length(queue_t *queue);

#ifdef __cplusplus
}
#endif
