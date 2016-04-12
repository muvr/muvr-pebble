#include "compat.h"
#include "am.h"
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
    uint32_t count;
    // the last error code
    int last_error;
    // the number of packets sent since encountering error
    int last_error_distance;
    // the number of errors
    int error_count;
    // transmission in progress
    bool send_in_progress;

    // the header fields
    uint32_t type;
    uint8_t sample_size;
    uint8_t samples_per_second;
    uint8_t sequence_number;
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
    DictionaryIterator *message;
    AppMessageResult app_message_result;
    if ((app_message_result = app_message_outbox_begin(&message)) != APP_MSG_OK) {
        context->last_error = -OUTB_B_CODE - app_message_result;

        return false;
    }

    DictionaryResult dictionary_result;
    if ((dictionary_result = dict_write_data(message, key, buffer, size)) != DICT_OK) {
        context->last_error = -DICT_W_CODE - dictionary_result;

        return false;
    }
    if ((dictionary_result = dict_write_int32(message, 0x0c000000, context->count)) != DICT_OK) {
        context->last_error = -DICT_W_CODE - dictionary_result;

        return false;
    }

    dict_write_end(message);

    if ((app_message_result = app_message_outbox_send()) != APP_MSG_OK) {
        context->last_error = -OUTB_S_CODE - app_message_result;

        return false;
    }

    return true;
}

static void send_message(const uint32_t key, const uint8_t* payload_buffer, const uint16_t size, const double timestamp, const uint16_t duration) {
    static const uint16_t payload_size_max = APP_MESSAGE_OUTBOX_SIZE - sizeof(struct header);

    struct am_context_t *context = app_message_get_context();
    if (context == NULL) return;
    if (context->send_in_progress) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "send_message: dropped message.");
        return;
    }

    context->send_in_progress = true;

    if (size > payload_size_max) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "send_message: needed %d, had %d B.", size, payload_size_max);
        EXIT(-3);
    }

    uint8_t message_buffer[APP_MESSAGE_OUTBOX_SIZE];
    memcpy(message_buffer + sizeof(struct header), payload_buffer, size);
    for (int i = 0; i < 5; ++i) {
        struct header *header = (struct header *) message_buffer;
        header->preamble1 = 0x61;
        header->preamble2 = 0x65;
        header->types_count = 1;
        header->samples_per_second = context->samples_per_second;
        header->timestamp = timestamp;
        header->count = (uint32_t) (size / context->sample_size) * 3; // number of values
        header->type = context->type;

        // header->sequence_number = context->sequence_number;
        // header->duration = duration;

        if (send_buffer(context, key, message_buffer, (uint16_t) (size + sizeof(struct header)))) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "send_message: sent %lu samples, at %g", header->count, timestamp);
            context->count++;
            break;
        } else {
            char err[20];
            get_error_text(context->last_error, err, 20);
            APP_LOG(APP_LOG_LEVEL_DEBUG, "send_message: not sent: %s. Size: %u", err, (uint16_t) (size + sizeof(struct header)));
            psleep(200);
        }
    }
    context->sequence_number++;
    context->send_in_progress = false;
}

__unused // not really, it's used in main.c
void am_send_simple(const msgkey_t key, const uint8_t value) {
    struct am_context_t *context = app_message_get_context();
    if (context == NULL) return;
    if (context->send_in_progress) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "send_message: dropped message.");
        return;
    }

    context->send_in_progress = true;
    for (int i = 0; i < 5; ++i) {
        uint8_t message_buffer[1] = {value};
        if (send_buffer(context, key, message_buffer, 1)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "am_send_simple: sent.");
            context->count++;
            break;
        } else {
            char err[20];
            get_error_text(context->last_error, err, 20);
            APP_LOG(APP_LOG_LEVEL_DEBUG, "send_message: not sent: %s", err);
            psleep(200);
        }
    }
    context->send_in_progress = false;
}

void sample_callback(const uint8_t* payload_buffer, const uint16_t size, const double timestamp, const uint16_t duration) {
    send_message(msg_ad, payload_buffer, size, timestamp, duration);
}

static void send_failed(DictionaryIterator __unused *iterator, AppMessageResult __unused reason, void __unused *context) {

}

message_callback_t am_start(uint32_t type, uint8_t samples_per_second, uint8_t sample_size) {
    struct am_context_t *context = malloc(sizeof(struct am_context_t));
    context->count = 0;
    context->error_count = 0;
    context->last_error = 0;
    context->last_error_distance = 0;

    context->type = type;
    context->sample_size = sample_size;
    context->samples_per_second = samples_per_second;
    context->sequence_number = 0;
    context->send_in_progress = false;

    app_message_set_context(context);
    app_message_register_outbox_failed(send_failed);

    return &sample_callback;
}

void am_stop() {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "am_stop() stopping...");

    struct am_context_t *context = app_message_get_context();

    uint8_t buffer[1] = {0};
    send_message(msg_dead, buffer, 1, 0, 0);

    free(context);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "am_stop() stopped.");
}

__unused // not really, it's used in main.c
void am_get_status(char *text, uint16_t max_size) {
    struct am_context_t *context = app_message_get_context();
    if (context == NULL) {
        strncpy(text, "Not recording", max_size);
    } else {
        char error_text[16];
        get_error_text(context->last_error, error_text, 16);
        snprintf(text, max_size, "C: %ld\nLE: %d %s\nLED: %d\nEC: %d\nUB: %d",
                 context->count,
                 context->last_error, error_text, context->last_error_distance, context->error_count,
                 (int)heap_bytes_used());
    }
}
