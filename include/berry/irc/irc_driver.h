#ifndef IRC_DRIVER_H
#define IRC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/* Tipe return untuk fungsi IRC */
#define WINEIRCcode int

/* Struktur handle untuk koneksi IRC */
typedef struct _WINEIRC_handle {
    int socket_fd;      /* Socket descriptor */
    char *server;       /* Nama/Alamat server IRC */
    int port;           /* Port server */
    char *nick;         /* Nickname bot */
    char *user;         /* User string (termasuk parameter USER) */
    char *channel;      /* Channel yang akan di-join */
    int is_connected;   /* Status koneksi */
} WINEIRC_handle;

/* Inisialisasi global (jika diperlukan) */
WINEIRCcode WINEIRC_global_init(void);

/* Cleanup global resources (jika diperlukan) */
WINEIRCcode WINEIRC_global_cleanup(void);

/* Membuat koneksi dan login ke server IRC serta join channel */
WINEIRC_handle* WINEIRC_create(const char* server, int port,
                               const char* nick,
                               const char* user,
                               const char* channel);

/* Join channel IRC yang telah dikonfigurasi dalam handle */
WINEIRCcode WINEIRC_join_channel(WINEIRC_handle* handle);

/* Mengirim pesan ke channel yang sudah di-join */
WINEIRCcode WINEIRC_send_message(WINEIRC_handle* handle, const char* message);

/* Fungsi keep-alive alternatif: memonitor koneksi
   dan jika koneksi hilang, akan mencoba reconnect dan join kembali */
WINEIRCcode WINEIRC_keep_alive(WINEIRC_handle* handle);

/* Disconnect dari server IRC */
WINEIRCcode WINEIRC_disconnect(WINEIRC_handle* handle);

/* Membebaskan memori dan resource pada handle IRC */
void WINEIRC_free(WINEIRC_handle* handle);

#ifdef __cplusplus
}
#endif

#endif // IRC_DRIVER_H