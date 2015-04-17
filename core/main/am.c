#include "compat.h"
#include "am.h"
#include "fixed_queue.h"
#include <pebble.h>

#define OUTB_B_CODE 32768
#define DICT_W_CODE 65535
#define OUTB_S_CODE 98304
#define OUTB_F_CODE 131072

/**
 * Context that holds the current callback and samples_per_second. It is used in the accelerometer
 * callback to calculate the G forces and to push the packed sample buffer to the callback.
 */
struct am_context_t {
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
};

static char *get_error_text(int code, char *result, size_t size) {
    if (size < 5) return "";
    if (size < 10) return strcpy(result, "E_MEM");

    strcpy(result, "----- ");
    if (code < -OUTB_F_CODE)      { strcpy(result, "outb-f "); code += OUTB_F_CODE; }
    else if (code < -OUTB_S_CODE) { strcpy(result, "outb-s "); code += OUTB_S_CODE; }
    else if (code < -DICT_W_CODE) { strcpy(result, "dict-w "); code += DICT_W_CODE; }
    else if (code < -OUTB_B_CODE) { strcpy(result, "outb-b "); code += OUTB_B_CODE; }
    switch (-code) {
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

static bool send_buffer(struct am_context_t *context, const uint32_t key, const uint8_t *buffer, const uint16_t size) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "send_buffer(..., %lu, ..., %d)", key, size);

    DictionaryIterator *message;
    AppMessageResult app_message_result;
    if ((app_message_result = app_message_outbox_begin(&message)) != APP_MSG_OK) {
        //context->error_count++;
        //context->last_error_distance = 0;
        context->last_error = -OUTB_B_CODE - app_message_result;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "    app_message_outbox_begin failed %d", app_message_result);

        return false;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "    app_message_outbox_begin OK");
    DictionaryResult dictionary_result;
    if ((dictionary_result = dict_write_data(message, key, buffer, size)) != DICT_OK) {
        //context->error_count++;
        //context->last_error_distance = 0;
        context->last_error = -DICT_W_CODE - dictionary_result;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "    dict_write_data failed %d", dictionary_result);

        return false;
    }

    APP_LOG(APP_LOG_LEVEL_DEBUG, "    dict_write_data OK");
    dict_write_end(message);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "    dict_write_end OK");
    if (message == NULL) return false;

    if ((app_message_result = app_message_outbox_send()) != APP_MSG_OK) {
        //context->error_count++;
        //context->last_error_distance = 0;
        context->last_error = -OUTB_S_CODE - app_message_result;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "    app_message_outbox_send failed %d", app_message_result);

        return false;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "   app_message_outbox_send OK");

    return true;
}

void send_all_messages(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "send_all_messages(void)");
    static const uint16_t payload_size_max = APP_MESSAGE_OUTBOX_SIZE_MINIMUM - sizeof(struct header);
    struct am_context_t *context = app_message_get_context();
    if (context == NULL) return;
    if (context->queue == NULL) return;
    uint8_t *buffer = malloc(APP_MESSAGE_OUTBOX_SIZE_MINIMUM);

    uint16_t length;
    uint16_t last_length = 0;
    uint8_t same_counter = 0;
    while ((length = queue_length(context->queue)) > 0) {
        psleep(1);
        if (last_length == length) same_counter++;
        if (same_counter > 10) {
            // this is now an error.
            context->error_count++;
            break;
        }
        last_length = length;

        uint32_t key;
        uint64_t timestamp;
        uint16_t payload_size = queue_peek(context->queue, &key, buffer + sizeof(struct header), payload_size_max, &timestamp);
        if (payload_size > payload_size_max) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "send_all_messages(void): needed %d, had %d B.", payload_size, payload_size_max);
            return;
        }

        struct header *header = (struct header *) buffer;
        header->type = context->type;
        header->samples_per_second = context->samples_per_second;
        header->sample_size = context->sample_size;
        header->count = (uint8_t) (payload_size / context->sample_size);
        header->queue_size = (uint8_t) (queue_length(context->queue) - 1);
        header->timestamp = timestamp;

        if (send_buffer(context, key, buffer, (uint16_t) (payload_size + sizeof(struct header)))) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent at %ld%ld", (uint32_t)(timestamp >> 32), (uint32_t)(timestamp & 0x7fffffff));
            queue_tail(context->queue);
            context->count++;
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent; queue_length = %d\n", queue_length(context->queue));
        } else {
            char err[20];
            get_error_text(context->last_error, err, 20);
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Not sent: %s; queue_length = %d\n", err, queue_length(context->queue));
            psleep(400);
        }
    }
    free(buffer);
}

void sample_callback(const uint8_t* buffer, const uint16_t size, const uint64_t timestamp) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "sample_callback(..., %d)", size);
    struct am_context_t *context = app_message_get_context();
    if (context == NULL) return;
    if (context->queue == NULL) return;

    queue_add(context->queue, 0xface0fb0, buffer, size, timestamp);
    send_all_messages();
}

void send_failed(DictionaryIterator __unused *iterator, AppMessageResult __unused reason, void __unused *context) {
    send_all_messages();
}

message_callback_t am_start(uint8_t type, uint8_t samples_per_second, uint8_t sample_size) {
    struct am_context_t *context = malloc(sizeof(struct am_context_t));
    context->queue = queue_create(8);
    context->count = 0;
    context->error_count = 0;
    context->last_error = 0;
    context->last_error_distance = 0;

    context->type = type;
    context->sample_size = sample_size;
    context->samples_per_second = samples_per_second;

    app_message_set_context(context);
    app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
    app_message_register_outbox_failed(send_failed);

    return &sample_callback;
}

void am_stop() {
    struct am_context_t *context = app_message_get_context();

    uint8_t buffer[1] = {0};
    queue_add(context->queue, 0xface0fb0, buffer, 1, 0);

    for (int i = 0; i < 10 && queue_length(context->queue) > 0; ++i) {
        send_all_messages();
        psleep(100);
    }

    queue_destroy(&context->queue);
    app_message_set_context(NULL);
    free(context);
}

__unused // not really, it's used in main.c
void am_get_status(char *text, uint16_t max_size) {
    struct am_context_t *context = app_message_get_context();
    if (context == NULL) {
        strncpy(text, "Not recording", max_size);
    } else {
        char error_text[16];
        get_error_text(context->last_error, error_text, 16);
        uint16_t ql = queue_length(context->queue);
        snprintf(text, max_size, "C: %d\nLE: %d %s\nLED: %d\nEC: %d\nQueue: %d\nUB: %d",
                 context->count,
                 context->last_error, error_text, context->last_error_distance, context->error_count, ql,
                 (int)heap_bytes_used());
    }
}
