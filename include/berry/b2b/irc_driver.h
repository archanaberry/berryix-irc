#ifndef IRC_DRIVER_H
#define IRC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

// Definisikan tipe return code untuk fungsi IRC
#define WINEIRCcode int

// Struktur handle untuk koneksi IRC
typedef struct {
    char* server;    // Alamat server IRC (misalnya "irc.libera.chat")
    int port;        // Nomor port (misalnya 6667)
    int sockfd;      // Socket file descriptor untuk koneksi TCP
    char* nick;      // Nickname yang digunakan
    char* user;      // Username yang digunakan
    char* channel;   // Channel yang akan di-join (misalnya "#imphnen")
} WINEIRC_handle;

// Inisialisasi global (jika diperlukan)
WINEIRCcode
WINEIRC_global_init(void);

// Membersihkan resource global (jika diperlukan)
void WINEIRC_global_cleanup(void);

// Membuat koneksi IRC dan mengirim perintah NICK/USER
WINEIRC_handle* WINEIRC_create(const char* server, int port, const char* nick, const char* user, const char* channel);

// Bergabung ke channel IRC
WINEIRCcode
WINEIRC_join_channel(WINEIRC_handle* handle);

// Mengirim pesan ke channel IRC (PRIVMSG)
WINEIRCcode
WINEIRC_send_message(WINEIRC_handle* handle, const char* message);

// Membersihkan/membebaskan handle dan menutup koneksi
void WINEIRC_free(WINEIRC_handle* handle);

#ifdef __cplusplus
}
#endif

#endif // IRC_DRIVER_H
