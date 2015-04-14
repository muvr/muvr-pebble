#include "am.h"

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

char *get_error_text(int code, char *result, size_t size) {
    if (size < 5) return "";
    if (size < 10) return strcpy(result, "E_MEM");

    if (code < -4000)      { strcpy(result, "outb-f "); code += 4000; }
    else if (code < -3000) { strcpy(result, "outb-s "); code += 3000; }
    else if (code < -2000) { strcpy(result, "dict-w "); code += 2000; }
    else if (code < -1000) { strcpy(result, "outb-b "); code += 1000; }
    switch (code) {
        case APP_MSG_OK:                          return strcat(result, "");
        case APP_MSG_SEND_TIMEOUT:                return strcat(result, "TO");
        case APP_MSG_SEND_REJECTED:               return strcat(result, "REJ");
        case APP_MSG_NOT_CONNECTED:               return strcat(result, "NC");
        case APP_MSG_APP_NOT_RUNNING:             return strcat(result, "NR");
        case APP_MSG_INVALID_ARGS:                return strcat(result, "INV");
        case APP_MSG_BUSY:                        return strcat(result, "BUSY");
        case APP_MSG_BUFFER_OVERFLOW:             return strcat(result, "OVER");
        case APP_MSG_ALREADY_RELEASED:            return strcat(result, "ARED");
        case APP_MSG_CALLBACK_ALREADY_REGISTERED: return strcat(result, "AREG");
        case APP_MSG_CALLBACK_NOT_REGISTERED:     return strcat(result, "CNR");
        case APP_MSG_OUT_OF_MEMORY:               return strcat(result, "MEMO");
        case APP_MSG_CLOSED:                      return strcat(result, "CL");
        case APP_MSG_INTERNAL_ERROR:              return strcat(result, "INTE");
        default:                                  return strcat(result, "UNK");
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
        context->last_error = -3000 - app_message_result;

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
    context->last_error = -4000 - reason;

    while (queue_length(context->queue) > 10) pop_message();

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
    char error_text[16];
    get_error_text(context->last_error, error_text, 16);
    size_t ql = 0;
    if (context->queue != NULL) ql = queue_length(context->queue);
    snprintf(*text, max_size - 1, "LE: %d %s\nLED: %d\nEC: %d\nP: %d\nQueue: %d\nUB: %d",
             context->last_error, error_text, context->last_error_distance, context->error_count, ql, heap_bytes_used());
}
