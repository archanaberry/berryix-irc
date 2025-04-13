#include "irc_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

struct IRCHandle {
    int sockfd;
};

IRCHandle* irc_connect(const ChatConfig* config) {
    printf("[DEBUG] %s: Baris %d - Memulai koneksi ke server IRC...\n", __FUNCTION__, __LINE__);

    // Alokasikan memory untuk handle
    IRCHandle *handle = malloc(sizeof(IRCHandle));
    if (!handle) {
        fprintf(stderr, "[DEBUG] %s: Baris %d - Gagal alokasikan memori untuk handle.\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    struct sockaddr_in server_addr;
    handle->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (handle->sockfd < 0) {
        perror("Socket creation failed");
        free(handle);
        return NULL;
    }
    printf("[DEBUG] %s: Baris %d - Socket berhasil dibuat.\n", __FUNCTION__, __LINE__);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->port);
    if (inet_pton(AF_INET, config->server, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        free(handle);
        return NULL;
    }
    printf("[DEBUG] %s: Baris %d - Alamat server berhasil diubah.\n", __FUNCTION__, __LINE__);

    if (connect(handle->sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(handle->sockfd);
        free(handle);
        return NULL;
    }
    printf("[DEBUG] %s: Baris %d - Koneksi berhasil dibuat ke server IRC.\n", __FUNCTION__, __LINE__);

    // Kirim perintah login IRC
    char buffer[256];

    snprintf(buffer, sizeof(buffer), "NICK %s\r\n", config->username);
    ssize_t sent = send(handle->sockfd, buffer, strlen(buffer), 0);
    if (sent < 0) {
        fprintf(stderr, "[DEBUG] %s: Baris %d - Gagal mengirim perintah NICK\n", __FUNCTION__, __LINE__);
        close(handle->sockfd);
        free(handle);
        return NULL;
    }
    printf("[DEBUG] %s: Baris %d - Mengirimkan perintah NICK: %s\n", __FUNCTION__, __LINE__, config->username);

    snprintf(buffer, sizeof(buffer), "USER %s 0 * :%s\r\n", config->username, config->username);
    sent = send(handle->sockfd, buffer, strlen(buffer), 0);
    if (sent < 0) {
        fprintf(stderr, "[DEBUG] %s: Baris %d - Gagal mengirim perintah USER\n", __FUNCTION__, __LINE__);
        close(handle->sockfd);
        free(handle);
        return NULL;
    }
    printf("[DEBUG] %s: Baris %d - Mengirimkan perintah USER: %s\n", __FUNCTION__, __LINE__, config->username);

    // Tunggu respons dari server IRC
    char response[512];
    ssize_t recv_len = recv(handle->sockfd, response, sizeof(response) - 1, 0);
    if (recv_len < 0) {
        fprintf(stderr, "[DEBUG] %s: Baris %d - Gagal menerima respons dari server\n", __FUNCTION__, __LINE__);
        close(handle->sockfd);
        free(handle);
        return NULL;
    }
    response[recv_len] = '\0';  // Pastikan null-terminated
    printf("[DEBUG] %s: Baris %d - Respons dari server IRC: %s\n", __FUNCTION__, __LINE__, response);

    return handle;
}

void irc_send_message(IRCHandle* handle, const char *channel, const char *message) {
    printf("[DEBUG] %s: Baris %d - Mengirim pesan ke channel %s: %s\n", __FUNCTION__, __LINE__, channel, message);
    if (!handle) return;
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "PRIVMSG %s :%s\r\n", channel, message);
    ssize_t sent = send(handle->sockfd, buffer, strlen(buffer), 0);
    if (sent < 0) {
        fprintf(stderr, "[DEBUG] %s: Baris %d - Gagal mengirim pesan\n", __FUNCTION__, __LINE__);
    }
}

void irc_disconnect(IRCHandle* handle) {
    printf("[DEBUG] %s: Baris %d - Memutuskan koneksi IRC.\n", __FUNCTION__, __LINE__);
    if (!handle) return;
    close(handle->sockfd);
    free(handle);
}