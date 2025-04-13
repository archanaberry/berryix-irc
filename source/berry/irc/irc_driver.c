#include "irc_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

/* --- Global Init & Cleanup --- */

/* Untuk sistem POSIX tidak diperlukan inisialisasi khusus,
   namun fungsi ini disediakan untuk konsistensi */
WINEIRCcode WINEIRC_global_init(void) {
    return 0;
}

WINEIRCcode WINEIRC_global_cleanup(void) {
    return 0;
}

/* --- Fungsi Helper: Membuat koneksi TCP ke server IRC --- */
static int create_connection(const char* server, int port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server_host;

    server_host = gethostbyname(server);
    if (server_host == NULL) {
        fprintf(stderr, "Error: host tidak ditemukan: %s\n", server);
        return -1;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error membuka socket");
        return -1;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server_host->h_addr, server_host->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error koneksi");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

/* --- Membuat Handle IRC dan Melakukan Login serta Join Channel --- */
WINEIRC_handle* WINEIRC_create(const char* server, int port,
                               const char* nick,
                               const char* user,
                               const char* channel) {
    WINEIRC_handle* handle = malloc(sizeof(WINEIRC_handle));
    if (!handle)
        return NULL;

    handle->server = strdup(server);
    handle->port = port;
    handle->nick = strdup(nick);
    handle->user = strdup(user);
    handle->channel = strdup(channel);
    handle->is_connected = 0;

    handle->socket_fd = create_connection(server, port);
    if (handle->socket_fd < 0) {
        WINEIRC_free(handle);
        return NULL;
    }
    handle->is_connected = 1;

    /* Kirim perintah login IRC: NICK dan USER dengan parameter lengkap */
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "NICK %s\r\n", nick);
    send(handle->socket_fd, buffer, strlen(buffer), 0);
    snprintf(buffer, sizeof(buffer), "USER %s 0 * :%s\r\n", user, user);
    send(handle->socket_fd, buffer, strlen(buffer), 0);

    /* Langsung join ke channel */
    WINEIRC_join_channel(handle);

    return handle;
}

/* --- Mengirim Perintah JOIN ke Channel --- */
WINEIRCcode WINEIRC_join_channel(WINEIRC_handle* handle) {
    if (!handle || !handle->is_connected)
        return -1;
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "JOIN %s\r\n", handle->channel);
    if (send(handle->socket_fd, buffer, strlen(buffer), 0) < 0) {
        perror("Error mengirim perintah JOIN");
        return -1;
    }
    return 0;
}

/* --- Mengirim Pesan ke Channel IRC --- */
WINEIRCcode WINEIRC_send_message(WINEIRC_handle* handle, const char* message) {
    if (!handle || !handle->is_connected)
        return -1;
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "PRIVMSG %s :%s\r\n", handle->channel, message);
    if (send(handle->socket_fd, buffer, strlen(buffer), 0) < 0) {
        perror("Error mengirim pesan");
        return -1;
    }
    return 0;
}

/* --- Fungsi Keep-Alive Alternatif ---
     Fungsi ini memonitor koneksi menggunakan select().
     Jika koneksi terputus (misal karena tidak ada reply terhadap PING),
     maka koneksi akan di-reconnect secara otomatis, login ulang,
     dan join ke channel kembali.
     
     Perlu diperhatikan: pendekatan ini mengabaikan pesan PING,
     sehingga tidak membalas dengan PONG, melainkan mengandalkan mekanisme
     reconnect untuk menjaga kehadiran bot di channel. --- */
WINEIRCcode WINEIRC_keep_alive(WINEIRC_handle* handle) {
    if (!handle)
        return -1;
    char buffer[512];
    fd_set read_fds;
    struct timeval tv;
    int n;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(handle->socket_fd, &read_fds);
        tv.tv_sec = 10;   /* Timeout tiap 10 detik */
        tv.tv_usec = 0;

        n = select(handle->socket_fd + 1, &read_fds, NULL, NULL, &tv);
        if (n < 0) {
            perror("select error");
            break;
        } else if (n == 0) {
            /* Timeout: tidak ada data diterima, lanjutkan monitoring */
            continue;
        } else {
            if (FD_ISSET(handle->socket_fd, &read_fds)) {
                int bytes = recv(handle->socket_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                    /* Koneksi terputus, lakukan reconnect */
                    fprintf(stderr, "Koneksi hilang. Mencoba reconnect...\n");
                    close(handle->socket_fd);
                    handle->is_connected = 0;
                    /* Coba reconnect */
                    handle->socket_fd = create_connection(handle->server, handle->port);
                    if (handle->socket_fd < 0) {
                        fprintf(stderr, "Reconnect gagal. Coba lagi dalam 5 detik...\n");
                        sleep(5);
                        continue;
                    }
                    handle->is_connected = 1;
                    /* Login ulang dan join channel dengan perintah USER yang lengkap */
                    char init_cmd[512];
                    snprintf(init_cmd, sizeof(init_cmd), "NICK %s\r\n", handle->nick);
                    send(handle->socket_fd, init_cmd, strlen(init_cmd), 0);
                    snprintf(init_cmd, sizeof(init_cmd), "USER %s 0 * :%s\r\n", handle->user, handle->user);
                    send(handle->socket_fd, init_cmd, strlen(init_cmd), 0);
                    WINEIRC_join_channel(handle);
                    continue;
                }
                buffer[bytes] = '\0';
                /* Jika pesan dimulai dengan PING, abaikan (tanpa membalas) */
                if (strncmp(buffer, "PING", 4) == 0) {
                    fprintf(stdout, "Terima PING (diabaikan): %s", buffer);
                } else {
                    fprintf(stdout, "Server: %s", buffer);
                }
            }
        }
    }
    return 0;
}

/* --- Fungsi untuk Disconnect dari IRC --- */
WINEIRCcode WINEIRC_disconnect(WINEIRC_handle* handle) {
    if (!handle)
        return -1;
    if (handle->is_connected) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "QUIT\r\n");
        send(handle->socket_fd, buffer, strlen(buffer), 0);
        close(handle->socket_fd);
        handle->is_connected = 0;
    }
    return 0;
}

/* --- Membebaskan Resource Handle IRC --- */
void WINEIRC_free(WINEIRC_handle* handle) {
    if (!handle)
        return;
    if (handle->is_connected) {
        WINEIRC_disconnect(handle);
    }
    free(handle->server);
    free(handle->nick);
    free(handle->user);
    free(handle->channel);
    free(handle);
}