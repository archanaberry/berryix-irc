#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <json-c/json.h>
#include <sys/socket.h>
#include "irc_driver.h"  // Pastikan path header sesuai

/* Struktur konfigurasi untuk IRC */
typedef struct {
    char *server;
    int   port;
    char *nick;
    char *user;
    char *channel;
} Config;

/* Fungsi untuk memuat konfigurasi dari file JSON */
Config *load_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Gagal membuka file konfigurasi");
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    char *data = malloc(fsize + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    fread(data, 1, fsize, fp);
    data[fsize] = '\0';
    fclose(fp);
    
    json_object *jobj = json_tokener_parse(data);
    free(data);
    if (!jobj) {
        fprintf(stderr, "Gagal parse JSON\n");
        return NULL;
    }
    
    Config *cfg = malloc(sizeof(Config));
    if (!cfg) {
        json_object_put(jobj);
        return NULL;
    }
    
    json_object *jserver = json_object_object_get(jobj, "server");
    json_object *jport   = json_object_object_get(jobj, "port");
    json_object *jnick   = json_object_object_get(jobj, "nick");
    json_object *juser   = json_object_object_get(jobj, "user");
    json_object *jchannel= json_object_object_get(jobj, "channel");
    
    cfg->server  = strdup(json_object_get_string(jserver));
    cfg->port    = json_object_get_int(jport);
    cfg->nick    = strdup(json_object_get_string(jnick));
    cfg->user    = strdup(json_object_get_string(juser));
    cfg->channel = strdup(json_object_get_string(jchannel));
    
    json_object_put(jobj);
    return cfg;
}

/* Fungsi untuk membebaskan konfigurasi */
void free_config(Config *cfg) {
    if (!cfg) return;
    free(cfg->server);
    free(cfg->nick);
    free(cfg->user);
    free(cfg->channel);
    free(cfg);
}

/* Fungsi utama test IRC */
int main(void) {
    const char *config_filename = "config_irc.json";
    
    if (WINEIRC_global_init() != 0) {
        fprintf(stderr, "Global init gagal\n");
        return -1;
    }
    
    Config *cfg = load_config(config_filename);
    if (!cfg) {
        fprintf(stderr, "Gagal memuat konfigurasi\n");
        WINEIRC_global_cleanup();
        return -1;
    }
    
    printf("[+] Konfigurasi dimuat: Server=%s, Port=%d, Nick=%s, Channel=%s\n",
           cfg->server, cfg->port, cfg->nick, cfg->channel);
    
    /* Membuat handle koneksi IRC */
    WINEIRC_handle *handle = WINEIRC_create(cfg->server, cfg->port, cfg->nick, cfg->user, cfg->channel);
    if (!handle) {
        fprintf(stderr, "[-] Gagal membuat koneksi IRC\n");
        free_config(cfg);
        WINEIRC_global_cleanup();
        return -1;
    }
    
    printf("[+] Terhubung dan join ke channel %s\n", cfg->channel);
    
    /* Kirim pesan test ke channel */
    if (WINEIRC_send_message(handle, "Hello from IRC test!") != 0) {
        fprintf(stderr, "[-] Gagal mengirim pesan\n");
    } else {
        printf("[+] Pesan test terkirim\n");
    }
    
    /* Loop mendengarkan pesan selama 30 detik */
    printf("[+] Menerima pesan selama 30 detik...\n");
    char buffer[512];
    int bytes;
    time_t start = time(NULL);
    while (time(NULL) - start < 30) {
        /* Menggunakan MSG_DONTWAIT agar tidak blocking */
        bytes = recv(handle->socket_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("[Received] %s", buffer);
        }
        usleep(100000); // Delay 100 ms
    }
    
    printf("[+] Selesai mendengarkan pesan. Disconnect...\n");
    WINEIRC_disconnect(handle);
    WINEIRC_free(handle);
    free_config(cfg);
    WINEIRC_global_cleanup();
    return 0;
}