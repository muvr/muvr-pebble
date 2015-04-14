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

static int _am_count = 0;
static int _am_last_error = 0;
static char* _am_last_error_text = "";
static int _am_last_error_distance = 0;
static int _am_error_count = 0;
static uint32_t _am_tag = 0;

static queue_t *_am_message_queue;

char* _am_get_error_message(int code) {
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

void _pop_message() {
    APP_LOG_AM("_pop_message - begin");
    
    if (_am_message_queue != NULL) {
        uint8_t* buffer;
        uint16_t size;
        
        queue_pop(_am_message_queue, &buffer, &size);
        if (buffer != NULL)
            free(buffer);
    }
    
    APP_LOG_AM("_pop_message - end");
}

void _send_next_message() {
    APP_LOG_AM("_send_next_message - begin");
    
    if (_am_message_queue == NULL) {
        APP_LOG_AM("_send_next_message - no queue_t - end");
        return;
    }
    
    uint8_t* buffer;
    uint16_t size;
    queue_peek(_am_message_queue, &buffer, &size);
    if (buffer == NULL) {
        APP_LOG_AM("_send_next_message - no message - end");
        return;
    }
    
    APP_LOG_AM("_send_next_message - message size: %d", size);
    
    size_t length = queue_length(_am_message_queue);
    if (length > 0 && length < 256) {
        uint8_t offset = (uint8_t)(length - 1);
        // ad_update_time_offset((void *) buffer, offset);
    }

    DictionaryIterator* message;
    AppMessageResult app_message_result;
    if ((app_message_result = app_message_outbox_begin(&message)) != APP_MSG_OK) {
        
        if (app_message_result == APP_MSG_BUSY) {
            APP_LOG_AM("_send_next_message - busy - end");
            return;
        }
        
        _am_error_count++;
        _am_last_error_distance = 0;
        _am_last_error = -1000 - app_message_result;
        _am_last_error_text = _am_get_error_message(app_message_result);
        
        APP_LOG_AM("_send_next_message - error - end");
        return;
    }

    DictionaryResult dictionary_result;
    if ((dictionary_result = dict_write_data(message, 0xface0fb0, buffer, size)) != DICT_OK) {
        _am_error_count++;
        _am_last_error_distance = 0;
        _am_last_error = -2000 - dictionary_result;
        _am_last_error_text = _am_get_error_message(dictionary_result);
        
        APP_LOG_AM("_send_next_message - error - end");
        return;
    }
    dict_write_end(message);
    if (message == NULL) {
        APP_LOG_AM("_send_next_message - programming error - end");
        return;
    }
    
    if ((app_message_result = app_message_outbox_send()) != APP_MSG_OK) {
        _am_error_count++;
        _am_last_error_distance = 0;
        _am_last_error = -1100 - app_message_result;
        _am_last_error_text = _am_get_error_message(app_message_result);
        
        APP_LOG_AM("_send_next_message - error - end");
        return;
    }
    
    APP_LOG_AM("_send_next_message - end");
}

void _am_outbox_sent(DictionaryIterator *iterator, void *context) {
    APP_LOG_AM("_am_outbox_sent - begin");
    
    _pop_message();
    _am_count++;
    _am_last_error_distance++;
    _send_next_message();
    
    APP_LOG_AM("_am_outbox_sent - end");
}

void _am_outbox_failed(DictionaryIterator* iterator, AppMessageResult reason, void* context) {
    APP_LOG_AM("_am_outbox_failed - begin");
    
    _pop_message();
    _am_error_count++;
    _am_last_error_distance = 0;
    _am_last_error = -1200 - reason;
    _am_last_error_text = _am_get_error_message(reason);
    
    if (queue_length(_am_message_queue) > 10) { // Avoid out of memory situations.
        while (queue_length(_am_message_queue) > 0) _pop_message();
    }
    
    _send_next_message();
    
    APP_LOG_AM("_am_outbox_failed - end");
}

void _am_gfs_sample_callback(uint8_t* buffer, uint16_t size) {
    APP_LOG_AM("_am_gfs_sample_callback - begin");
    
    if (_am_message_queue == NULL) {
        APP_LOG_AM("_am_gfs_sample_callback - no queue_t - end");
        return;
    }
    
    queue_add(_am_message_queue, buffer, size);
    _send_next_message();
    
    APP_LOG_AM("_am_gfs_sample_callback - end");
}

message_callback_t am_start(uint8_t type, uint8_t samples_per_second, uint8_t sample_size) {
    _am_message_queue = queue_create();
    
    app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
    app_message_register_outbox_sent(_am_outbox_sent);
    app_message_register_outbox_failed(_am_outbox_failed);

    return &_am_gfs_sample_callback;
}

void am_stop() {
    APP_LOG_AM("am_stop - begin");
    
    queue_destroy(&_am_message_queue);
    
    for (int i = 0; i < 5; ++i) {
        DictionaryIterator *iter;
        if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
            dict_write_int8(iter, 0x0000dead, 1);
            dict_write_end(iter);
            if (app_message_outbox_send() == APP_MSG_OK) break;
        }
        psleep(500);
    }
    _am_last_error = 0;
    _am_last_error_distance = -1;
    _am_error_count = 0;
    _am_count = 0;
    
    APP_LOG_AM("am_stop - end");
}

void am_get_status(char **text, size_t max_size) {
    size_t ql = 0;
    if (_am_message_queue != NULL) ql = queue_length(_am_message_queue);
    snprintf(*text, max_size - 1, "LE: %d %s\nLED: %d\nEC: %d\nP: %d\nQueue: %d\nUB: %d",
             _am_last_error, _am_last_error_text, _am_last_error_distance, _am_error_count, ql, heap_bytes_used());
}
