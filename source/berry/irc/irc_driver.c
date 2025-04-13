#include "irc_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       // close()
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

// Inisialisasi global: untuk socket pada Linux biasanya tidak perlu inisialisasi khusus
WINEIRCcode WINEIRC_global_init(void) {
    return 0;
}

void WINEIRC_global_cleanup(void) {
    // Tidak diperlukan pembersihan global khusus pada Linux
}

// Fungsi helper: menghubungkan ke server IRC menggunakan getaddrinfo
static int connect_to_server(const char* server, int port) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    char port_str[6];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;      // IPv4 atau IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP

    snprintf(port_str, sizeof(port_str), "%d", port);

    if ((rv = getaddrinfo(server, port_str, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // Loop melalui hasil dan koneksikan ke yang pertama berhasil
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue;
        }
        break; // Berhasil terhubung
    }

    if (p == NULL) {
        fprintf(stderr, "Gagal terhubung ke %s:%d\n", server, port);
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

// Membuat handle IRC dan mengirim perintah NICK dan USER
WINEIRC_handle* WINEIRC_create(const char* server, int port, const char* nick, const char* user, const char* channel) {
    WINEIRC_handle* handle = malloc(sizeof(WINEIRC_handle));
    if (!handle) {
        perror("malloc");
        return NULL;
    }

    handle->server = strdup(server);
    handle->port = port;
    handle->nick = strdup(nick);
    handle->user = strdup(user);
    handle->channel = strdup(channel);
    handle->sockfd = connect_to_server(server, port);
    if (handle->sockfd < 0) {
        WINEIRC_free(handle);
        return NULL;
    }

    // Kirim perintah NICK
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "NICK %s\r\n", nick);
    if (send(handle->sockfd, buffer, strlen(buffer), 0) == -1) {
        perror("send NICK");
    }

    // Kirim perintah USER
    snprintf(buffer, sizeof(buffer), "USER %s 0 * :%s\r\n", user, user);
    if (send(handle->sockfd, buffer, strlen(buffer), 0) == -1) {
        perror("send USER");
    }

    return handle;
}

// Bergabung ke channel (mengirim perintah JOIN)
WINEIRCcode WINEIRC_join_channel(WINEIRC_handle* handle) {
    if (!handle) return -1;
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "JOIN %s\r\n", handle->channel);
    if (send(handle->sockfd, buffer, strlen(buffer), 0) == -1) {
        perror("send JOIN");
        return -1;
    }
    return 0;
}

// Mengirim pesan ke channel dengan perintah PRIVMSG
WINEIRCcode WINEIRC_send_message(WINEIRC_handle* handle, const char* message) {
    if (!handle) return -1;
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "PRIVMSG %s :%s\r\n", handle->channel, message);
    if (send(handle->sockfd, buffer, strlen(buffer), 0) == -1) {
        perror("send PRIVMSG");
        return -1;
    }
    return 0;
}

// Membersihkan dan menutup koneksi IRC
void WINEIRC_free(WINEIRC_handle* handle) {
    if (!handle) return;
    if (handle->sockfd >= 0)
        close(handle->sockfd);
    free(handle->server);
    free(handle->nick);
    free(handle->user);
    free(handle->channel);
    free(handle);
}
