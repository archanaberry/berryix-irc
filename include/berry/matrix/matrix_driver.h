#ifndef MATRIX_DRIVER_H
#define MATRIX_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Jika belum didefinisikan, WINEMATRIXcode didefinisikan sebagai macro kosong.
   Macro ini dapat digunakan untuk mengatur visibility export bila diperlukan. */
#ifndef WINEMATRIXcode
#define WINEMATRIXcode
#endif

/**
 * @brief Struktur handle utama untuk koneksi Matrix.
 *
 * Berisi informasi tentang homeserver, username, password, dan access_token
 * yang diperoleh dari proses login.
 */
typedef struct {
    char *homeserver;     ///< URL homeserver Matrix, misal "https://matrix.org"
    char *username;       ///< ID pengguna Matrix (contoh: "@user:matrix.org")
    char *password;       ///< Password pengguna
    char *access_token;   ///< Token akses yang didapatkan setelah login
} WINEMATRIX_handle;

/**
 * @brief Inisialisasi global untuk library Matrix driver.
 *
 * Fungsi ini memanggil curl_global_init() sehingga harus dipanggil sebelum
 * fungsi-fungsi lain digunakan.
 *
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_global_init(void);

/**
 * @brief Membersihkan resource global yang digunakan oleh library.
 *
 * Fungsi ini memanggil curl_global_cleanup() dan harus dipanggil saat aplikasi
 * selesai menggunakan library ini.
 */
WINEMATRIXcode
void WINEMATRIX_global_cleanup(void);

/**
 * @brief Membuat handle baru dan melakukan login ke Matrix.
 *
 * Fungsi ini mengakses endpoint /_matrix/client/r0/login untuk melakukan
 * autentikasi menggunakan metode m.login.password.
 *
 * @param homeserver URL homeserver Matrix.
 * @param username ID pengguna Matrix.
 * @param password Password pengguna.
 * @return WINEMATRIX_handle* Pointer ke handle baru jika berhasil, NULL jika gagal.
 */
WINEMATRIXcode
WINEMATRIX_handle* WINEMATRIX_create(const char* homeserver, const char* username, const char* password);

/**
 * @brief Bergabung ke room Matrix.
 *
 * Mengakses endpoint /_matrix/client/r0/join/{roomId}?access_token=... untuk
 * melakukan join ke room tertentu.
 *
 * @param handle Pointer ke handle yang valid.
 * @param room_id ID room Matrix yang akan diikuti.
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_join_room(WINEMATRIX_handle* handle, const char* room_id);

/**
 * @brief Mengirim pesan ke room Matrix.
 *
 * Menggunakan endpoint /_matrix/client/r0/rooms/{roomId}/send/m.room.message/{txnId}?access_token=...
 * untuk mengirim pesan teks.
 *
 * @param handle Pointer ke handle yang valid.
 * @param room_id ID room tujuan.
 * @param message Pesan teks yang akan dikirim.
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_send_message(WINEMATRIX_handle* handle, const char* room_id, const char* message);

/**
 * @brief Mengirim pesan reply dengan mengutip pesan asli.
 *
 * Fungsi ini mengirim reply dengan menambahkan kutipan (prefiks "> ") dari pesan asli,
 * sesuai dengan struktur reply pada Matrix.
 *
 * @param handle Pointer ke handle yang valid.
 * @param room_id ID room tempat pesan dikirim.
 * @param original_event_id ID event pesan asli yang dikutip.
 * @param original_message Isi pesan asli.
 * @param reply_message Pesan balasan.
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_send_reply(WINEMATRIX_handle* handle, const char* room_id, const char* original_event_id,
                            const char* original_message, const char* reply_message);

/**
 * @brief Mengirim pesan direct message (DM).
 *
 * Asumsi bahwa room DM sudah ada.
 *
 * @param handle Pointer ke handle yang valid.
 * @param dm_room_id ID room DM.
 * @param message Pesan teks yang dikirim.
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_send_direct_message(WINEMATRIX_handle* handle, const char* dm_room_id, const char* message);

/**
 * @brief Mengirim reaction terhadap pesan tertentu.
 *
 * Menggunakan endpoint untuk m.reaction dengan tipe event m.annotation.
 *
 * @param handle Pointer ke handle yang valid.
 * @param room_id ID room tempat pesan berada.
 * @param target_event_id ID event pesan yang direaksikan.
 * @param reaction Emoji atau string reaction (misal: "👍").
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_send_reaction(WINEMATRIX_handle* handle, const char* room_id, const char* target_event_id, const char* reaction);

/**
 * @brief Menyematkan (pin) pesan di room Matrix.
 *
 * Mengakses endpoint state untuk mengatur pinned events.
 *
 * @param handle Pointer ke handle yang valid.
 * @param room_id ID room.
 * @param event_id ID pesan yang ingin dipin.
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_pin_message(WINEMATRIX_handle* handle, const char* room_id, const char* event_id);

/**
 * @brief Menghapus (redact) pesan dari room Matrix.
 *
 * Menggunakan endpoint /redact untuk menghapus pesan.
 *
 * @param handle Pointer ke handle yang valid.
 * @param room_id ID room.
 * @param event_id ID pesan yang akan dihapus.
 * @param reason Alasan penghapusan.
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_redact_message(WINEMATRIX_handle* handle, const char* room_id, const char* event_id, const char* reason);

/**
 * @brief Forward (bagikan) pesan ke room lain dengan menyertakan kutipan isi pesan asli.
 *
 * Mengirim pesan ke room tujuan dengan menyertakan informasi room dan event asli.
 *
 * @param handle Pointer ke handle yang valid.
 * @param dest_room_id ID room tujuan.
 * @param original_room_id ID room asal pesan.
 * @param original_event_id ID event pesan asli.
 * @param original_message Isi pesan asli.
 * @return int 0 jika berhasil, non-0 jika gagal.
 */
WINEMATRIXcode
int WINEMATRIX_forward_message(WINEMATRIX_handle* handle,
                               const char* dest_room_id,
                               const char* original_room_id,
                               const char* original_event_id,
                               const char* original_message);

/**
 * @brief Membebaskan memori yang digunakan oleh handle.
 *
 * @param handle Pointer ke WINEMATRIX_handle yang akan dibebaskan.
 */
WINEMATRIXcode
void WINEMATRIX_free(WINEMATRIX_handle* handle);

#ifdef __cplusplus
}
#endif

#endif /* MATRIX_DRIVER_H */