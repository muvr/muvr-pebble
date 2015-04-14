#include "am.h"

#define APP_LOG_AM APP_LOG_DEBUG

#define GFS_HEADER_TYPE (uint16_t)0xad

/*
void ad_update_time_offset(void *header, uint8_t padding) {
    struct header *h = (struct header *) header;
    h->time_offset = padding;
}

void ad_write_header() {
    struct header *h = (struct header *) ad_context.buffer;
    h->type = GFS_HEADER_TYPE;
    h->count = 0;
    h->sample_size = sizeof(struct threed_data);
    h->samples_per_second = ad_context.samples_per_second;
    ad_context.buffer_position = sizeof(struct header);
}
*/

/**
 * Context that holds the current callback and samples_per_second. It is used in the accelerometer
 * callback to calculate the G forces and to push the packed sample buffer to the callback.
 */
typedef struct am_context_t {
    // the number of packets sent
    int count;
    // the last error code
    int last_error;
    // the number of packets sent since encountering error
    int last_error_distance;
    // the number of errors
    int error_count;
    // the message queue
    queue_t *queue;

    // the header fields
    uint8_t type;
    uint8_t sample_size;
    uint8_t samples_per_second;
    uint8_t time_offset;
};

char *get_error_text(int code) {
    switch (code) {
        case APP_MSG_OK: return "";
        case APP_MSG_SEND_TIMEOUT: return "TO";
        case APP_MSG_SEND_REJECTED: return "REJ";
        case APP_MSG_NOT_CONNECTED: return "NC";
        case APP_MSG_APP_NOT_RUNNING: return "NR";
        case APP_MSG_INVALID_ARGS: return "INV";
        case APP_MSG_BUSY: return "BUSY";
        case APP_MSG_BUFFER_OVERFLOW: return "OVER";
        case APP_MSG_ALREADY_RELEASED: return "ARED";
        case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "AREG";
        case APP_MSG_CALLBACK_NOT_REGISTERED: return "CNR";
        case APP_MSG_OUT_OF_MEMORY: return "MEMO";
        case APP_MSG_CLOSED: return "CL";
        case APP_MSG_INTERNAL_ERROR: return "INTE";
        default: return "UNK";
    }
}

void pop_message(void) {
    struct am_context_t *context = app_message_get_context();
    if (context->queue == NULL) return;

    uint8_t* buffer;
    uint16_t size;
    queue_pop(context->queue, &buffer, &size);
    if (buffer != NULL) free(buffer);
}

void send_buffer(struct am_context_t *context, uint8_t *buffer, uint16_t size) {
    DictionaryIterator* message;
    AppMessageResult app_message_result;
    if ((app_message_result = app_message_outbox_begin(&message)) != APP_MSG_OK) {

        if (app_message_result == APP_MSG_BUSY) return;

        context->error_count++;
        context->last_error_distance = 0;
        context->last_error = -1000 - app_message_result;

        return;
    }

    DictionaryResult dictionary_result;
    if ((dictionary_result = dict_write_data(message, 0xface0fb0, buffer, size)) != DICT_OK) {
        context->error_count++;
        context->last_error_distance = 0;
        context->last_error = -2000 - dictionary_result;

        return;
    }
    dict_write_end(message);
    if (message == NULL) return;

    if ((app_message_result = app_message_outbox_send()) != APP_MSG_OK) {
        context->error_count++;
        context->last_error_distance = 0;
        context->last_error = -1100 - app_message_result;

        return;
    }
}

void send_next_message() {
    struct am_context_t *context = app_message_get_context();
    if (context->queue == NULL) return;

    uint8_t *payload_buffer;
    uint16_t payload_size;
    queue_peek(context->queue, &payload_buffer, &payload_size);
    if (payload_buffer == NULL) return;

    uint16_t size = (uint16_t)(payload_size + sizeof(struct header));
    uint8_t *buffer = malloc(size);
    memcpy(buffer + sizeof(struct header), payload_buffer, payload_size);
    struct header *header = (struct header *)buffer;
    header->type = context->type;
    header->samples_per_second = context->samples_per_second;
    header->sample_size = context->sample_size;
    header->count = (uint8_t)(payload_size / context->sample_size);
    header->time_offset = 0;
    send_buffer(context, buffer, size);
    free(buffer);
}

void outbox_sent(DictionaryIterator *iterator, void *ctx) {
    struct am_context_t *context = ctx;
    pop_message();
    context->count++;
    context->last_error_distance++;
    send_next_message();
}

void outbox_failed(DictionaryIterator* iterator, AppMessageResult reason, void* ctx) {
    struct am_context_t *context = ctx;

    pop_message();
    context->error_count++;
    context->last_error_distance = 0;
    context->last_error = -1200 - reason;

    if (queue_length(context->queue) > 10) { // Avoid out of memory situations.
        while (queue_length(context->queue) > 0) pop_message();
    }

    send_next_message();
}

void sample_callback(uint8_t* buffer, uint16_t size) {
    struct am_context_t *context = app_message_get_context();
    if (context->queue == NULL) return;

    queue_add(context->queue, buffer, size);
    send_next_message();
}

message_callback_t am_start(uint8_t type, uint8_t samples_per_second, uint8_t sample_size) {
    struct am_context_t *context = malloc(sizeof(struct am_context_t));
    context->queue = queue_create();
    context->count = 0;
    context->error_count = 0;
    context->last_error = 0;
    context->last_error_distance = 0;

    context->type = type;
    context->sample_size = sample_size;
    context->samples_per_second = samples_per_second;

    app_message_set_context(context);
    app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
    app_message_register_outbox_sent(outbox_sent);
    app_message_register_outbox_failed(outbox_failed);

    return &sample_callback;
}

void am_stop() {
    struct am_context_t *context = app_message_get_context();
    queue_destroy(&context->queue);
    app_message_set_context(NULL);

    for (int i = 0; i < 5; ++i) {
        DictionaryIterator *iter;
        if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
            dict_write_int8(iter, 0x0000dead, 1);
            dict_write_end(iter);
            if (app_message_outbox_send() == APP_MSG_OK) break;
        }
        psleep(500);
    }

    free(context);

}

void am_get_status(char **text, size_t max_size) {
    struct am_context_t *context = app_message_get_context();
    size_t ql = 0;
    if (context->queue != NULL) ql = queue_length(context->queue);
    char *error_text = get_error_text(context->last_error);
    snprintf(*text, max_size - 1, "LE: %d %s\nLED: %d\nEC: %d\nP: %d\nQueue: %d\nUB: %d",
             context->last_error, error_text, context->last_error_distance, context->error_count, ql, heap_bytes_used());
}
