/* include/chatlib.h */
#ifndef CHATLIB_H
#define CHATLIB_H

typedef enum {
    CHAT_PROTOCOL_IRC,
    CHAT_PROTOCOL_MATRIX
} ChatProtocol;

typedef struct {
    int protocol;
    char* server;    // Changed to char* (non-const)
    int port;
    char* username;  // Changed to char* (non-const)
    char* password;  // Changed to char* (non-const)
    char* room;      // Changed to char* (non-const)
} ChatConfig;

/* Struktur abstrak untuk menyimpan handle koneksi */
typedef struct ChatHandle ChatHandle;

/* Membuat koneksi sesuai dengan protokol yang dipilih */
ChatHandle* chat_connect(const ChatConfig* config);
/* Mengirim pesan ke channel (IRC) atau room (Matrix) */
void chat_send_message(ChatHandle* handle, const char *channel, const char *message);
/* Memutus koneksi */
void chat_disconnect(ChatHandle* handle);

#endif // CHATLIB_H