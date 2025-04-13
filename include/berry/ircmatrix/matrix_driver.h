/* src/matrix_driver.h */
#ifndef MATRIX_DRIVER_H
#define MATRIX_DRIVER_H

#include "chatlib.h"

/* Handle khusus untuk koneksi Matrix */
typedef struct MatrixHandle MatrixHandle;

/* Membuat koneksi dan login ke server Matrix */
MatrixHandle* matrix_connect(const ChatConfig* config);
/* Mengirim pesan ke room Matrix */
void matrix_send_message(MatrixHandle* handle, const char *room, const char *message);
/* Memutus koneksi / membersihkan handle Matrix */
void matrix_disconnect(MatrixHandle* handle);

#endif // MATRIX_DRIVER_H