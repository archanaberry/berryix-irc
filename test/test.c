#include "chatlib.h"
#include <json-c/json.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fungsi untuk parsing protokol dari string ke enum
static ChatProtocol parse_protocol(const char *protocol_str) {
    if (strcmp(protocol_str, "irc") == 0) return CHAT_PROTOCOL_IRC;
    if (strcmp(protocol_str, "matrix") == 0) return CHAT_PROTOCOL_MATRIX;
    fprintf(stderr, "Protokol tidak valid: %s\n", protocol_str);
    return -1;
}

// Thread untuk mengirim pesan
void *send_message_thread(void *arg) {
    ChatConfig *config = (ChatConfig *)arg;
    ChatHandle *handle = chat_connect(config);
    
    if (!handle) {
        fprintf(stderr, "Gagal terhubung ke %s\n", 
            config->protocol == CHAT_PROTOCOL_IRC ? "IRC" : "Matrix");
    } else {
        const char *message = (config->protocol == CHAT_PROTOCOL_IRC) 
            ? "Halo saya dari IRC" 
            : "Halo saya dari MATRIX";
        
        printf("Mengirim pesan ke %s: %s\n", 
               config->protocol == CHAT_PROTOCOL_IRC ? "IRC" : "Matrix",
               message);
        chat_send_message(handle, config->room, message);
        chat_disconnect(handle);
    }
    
    // Bersihkan memori
    free(config->server);
    free(config->username);
    if (config->password) free(config->password);
    free(config->room);
    free(config);
    
    return NULL;
}

int main() {
    // Baca file konfigurasi
    json_object *root = json_object_from_file("config.json");
    if (!root) {
        fprintf(stderr, "Gagal membaca config.json\n");
        return 1;
    }
    
    // Ambil array configs
    json_object *configs_array;
    if (!json_object_object_get_ex(root, "configs", &configs_array)) {
        fprintf(stderr, "Key 'configs' tidak ditemukan\n");
        json_object_put(root);
        return 1;
    }
    
    // Validasi jumlah config
    int array_len = json_object_array_length(configs_array);
    if (array_len < 2) {
        fprintf(stderr, "Minimal 2 config (IRC + Matrix)\n");
        json_object_put(root);
        return 1;
    }
    
    // Buat thread untuk setiap config
    pthread_t threads[2];
    for (int i = 0; i < 2; i++) {
        json_object *config_obj = json_object_array_get_idx(configs_array, i);
        
        // Parse ChatConfig dari JSON
        ChatConfig *cfg = malloc(sizeof(ChatConfig));
        if (!cfg) {
            fprintf(stderr, "Gagal alokasi memori\n");
            continue;
        }
        
        // Protocol
        json_object *protocol_obj;
        if (!json_object_object_get_ex(config_obj, "protocol", &protocol_obj)) {
            fprintf(stderr, "Key 'protocol' tidak ditemukan\n");
            free(cfg);
            continue;
        }
        cfg->protocol = parse_protocol(json_object_get_string(protocol_obj));
        
        // Server
        json_object *server_obj;
        if (!json_object_object_get_ex(config_obj, "server", &server_obj)) {
            fprintf(stderr, "Key 'server' tidak ditemukan\n");
            free(cfg);
            continue;
        }
        cfg->server = strdup(json_object_get_string(server_obj));
        
        // Port
        json_object *port_obj;
        if (!json_object_object_get_ex(config_obj, "port", &port_obj)) {
            fprintf(stderr, "Key 'port' tidak ditemukan\n");
            free(cfg->server);
            free(cfg);
            continue;
        }
        cfg->port = json_object_get_int(port_obj);
        
        // Username
        json_object *username_obj;
        if (!json_object_object_get_ex(config_obj, "username", &username_obj)) {
            fprintf(stderr, "Key 'username' tidak ditemukan\n");
            free(cfg->server);
            free(cfg);
            continue;
        }
        cfg->username = strdup(json_object_get_string(username_obj));
        
        // Password (optional untuk IRC)
        json_object *password_obj;
        if (json_object_object_get_ex(config_obj, "password", &password_obj)) {
            cfg->password = strdup(json_object_get_string(password_obj));
        } else {
            cfg->password = NULL;
        }
        
        // Room
        json_object *room_obj;
        if (!json_object_object_get_ex(config_obj, "room", &room_obj)) {
            fprintf(stderr, "Key 'room' tidak ditemukan\n");
            free(cfg->server);
            free(cfg->username);
            if (cfg->password) free(cfg->password);
            free(cfg);
            continue;
        }
        cfg->room = strdup(json_object_get_string(room_obj));
        
        // Validasi password untuk Matrix
        if (cfg->protocol == CHAT_PROTOCOL_MATRIX && !cfg->password) {
            fprintf(stderr, "Matrix memerlukan password!\n");
            free(cfg->server);
            free(cfg->username);
            free(cfg->room);
            free(cfg);
            continue;
        }
        
        // Jalankan thread
        if (pthread_create(&threads[i], NULL, send_message_thread, cfg) != 0) {
            fprintf(stderr, "Gagal membuat thread\n");
            free(cfg->server);
            free(cfg->username);
            if (cfg->password) free(cfg->password);
            free(cfg->room);
            free(cfg);
        }
    }
    
    // Tunggu semua thread selesai
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }
    
    json_object_put(root);
    printf("Selesai mengirim pesan ke semua protokol\n");
    return 0;
}