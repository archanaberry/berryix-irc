/* src/chatlib.c */
#include <stdlib.h>
#include "chatlib.h"
#include "irc_driver.h"
#include "matrix_driver.h"

/* Definisi internal ChatHandle */
struct ChatHandle {
    ChatProtocol protocol;
    void *driver_handle; /* Pointer ke handle driver spesifik */
};

ChatHandle* chat_connect(const ChatConfig* config) {
    ChatHandle* handle = malloc(sizeof(ChatHandle));
    if (!handle) return NULL;

    handle->protocol = config->protocol;
    if (config->protocol == CHAT_PROTOCOL_IRC) {
        handle->driver_handle = irc_connect(config);
    } else if (config->protocol == CHAT_PROTOCOL_MATRIX) {
        handle->driver_handle = matrix_connect(config);
    } else {
        free(handle);
        return NULL;
    }
    return handle;
}

void chat_send_message(ChatHandle* handle, const char *channel, const char *message) {
    if (!handle) return;
    if (handle->protocol == CHAT_PROTOCOL_IRC) {
        irc_send_message(handle->driver_handle, channel, message);
    } else if (handle->protocol == CHAT_PROTOCOL_MATRIX) {
        matrix_send_message(handle->driver_handle, channel, message);
    }
}

void chat_disconnect(ChatHandle* handle) {
    if (!handle) return;
    if (handle->protocol == CHAT_PROTOCOL_IRC) {
        irc_disconnect(handle->driver_handle);
    } else if (handle->protocol == CHAT_PROTOCOL_MATRIX) {
        matrix_disconnect(handle->driver_handle);
    }
    free(handle);
}