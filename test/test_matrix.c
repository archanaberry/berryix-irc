#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <time.h>
#include "matrix_driver.h"

/* Struktur untuk menyimpan konfigurasi yang dibaca dari file JSON */
typedef struct {
    char *homeserver;
    char *username;
    char *password;
    char *room_id;
    char *access_token;
} Config;

/* Fungsi untuk memuat konfigurasi dari file config.json */
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
    cfg->homeserver   = strdup(json_object_get_string(json_object_object_get(jobj, "homeserver")));
    cfg->username     = strdup(json_object_get_string(json_object_object_get(jobj, "username")));
    cfg->password     = strdup(json_object_get_string(json_object_object_get(jobj, "password")));
    cfg->room_id      = strdup(json_object_get_string(json_object_object_get(jobj, "room_id")));
    
    json_object *jtoken = NULL;
    if (json_object_object_get_ex(jobj, "access_token", &jtoken))
        cfg->access_token = strdup(json_object_get_string(jtoken));
    else
        cfg->access_token = strdup("");

    json_object_put(jobj);
    return cfg;
}

/* Fungsi untuk menyimpan konfigurasi ke file config.json */
int save_config(const char *filename, Config *cfg) {
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "homeserver", json_object_new_string(cfg->homeserver));
    json_object_object_add(jobj, "username", json_object_new_string(cfg->username));
    json_object_object_add(jobj, "password", json_object_new_string(cfg->password));
    json_object_object_add(jobj, "room_id", json_object_new_string(cfg->room_id));
    json_object_object_add(jobj, "access_token", json_object_new_string(cfg->access_token));

    const char *json_str = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Gagal membuka file untuk menulis konfigurasi");
        json_object_put(jobj);
        return -1;
    }
    fprintf(fp, "%s\n", json_str);
    fclose(fp);
    json_object_put(jobj);
    return 0;
}

/* Fungsi pembantu untuk membersihkan konfigurasi */
void free_config(Config *cfg) {
    if (!cfg) return;
    free(cfg->homeserver);
    free(cfg->username);
    free(cfg->password);
    free(cfg->room_id);
    free(cfg->access_token);
    free(cfg);
}

/* Fungsi callback untuk menyimpan respons (digunakan di fungsi lain bila diperlukan) */
struct buffer {
    char *data;
    size_t size;
};

static size_t buffer_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    struct buffer *buf = (struct buffer *)userp;
    buf->data = realloc(buf->data, buf->size + total + 1);
    memcpy(&(buf->data[buf->size]), contents, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

/* Fungsi untuk mengirim reaction (memanfaatkan WINEMATRIX_send_reaction) */
void send_reaction(WINEMATRIX_handle *h, const char *room_id, const char *event_id, const char *emoji) {
    /* Contoh penggunaan fungsi dari library */
    WINEMATRIX_send_reaction(h, room_id, event_id, emoji);
}

/* Fungsi untuk mendengarkan pesan dan meresponnya */
void listen_and_respond(WINEMATRIX_handle *h, const char *room_id, const char *username) {
    char sync_token[1024] = {0};
    while (1) {
        CURL *curl = curl_easy_init();
        if (!curl) {
            sleep(2);
            continue;
        }

        char url[2048];
        if (strlen(sync_token) > 0) {
            snprintf(url, sizeof(url), "%s/_matrix/client/r0/sync?access_token=%s&since=%s&timeout=30000",
                     h->homeserver, h->access_token, sync_token);
        } else {
            snprintf(url, sizeof(url), "%s/_matrix/client/r0/sync?access_token=%s&timeout=30000",
                     h->homeserver, h->access_token);
        }

        struct buffer buf = {0};
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, buffer_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK || buf.size == 0) {
            free(buf.data);
            sleep(2);
            continue;
        }

        json_object *root = json_tokener_parse(buf.data);
        free(buf.data);
        if (!root)
            continue;

        json_object *next_batch;
        if (json_object_object_get_ex(root, "next_batch", &next_batch)) {
            strncpy(sync_token, json_object_get_string(next_batch), sizeof(sync_token)-1);
        }

        json_object *rooms, *join, *room_obj, *timeline, *events;
        if (json_object_object_get_ex(root, "rooms", &rooms) &&
            json_object_object_get_ex(rooms, "join", &join) &&
            json_object_object_get_ex(join, room_id, &room_obj) &&
            json_object_object_get_ex(room_obj, "timeline", &timeline) &&
            json_object_object_get_ex(timeline, "events", &events)) {

            int len = json_object_array_length(events);
            for (int i = 0; i < len; ++i) {
                json_object *event = json_object_array_get_idx(events, i);
                json_object *type, *content, *body, *sender, *event_id;
                if (json_object_object_get_ex(event, "type", &type) &&
                    strcmp(json_object_get_string(type), "m.room.message") == 0 &&
                    json_object_object_get_ex(event, "content", &content) &&
                    json_object_object_get_ex(content, "body", &body) &&
                    json_object_object_get_ex(event, "sender", &sender) &&
                    json_object_object_get_ex(event, "event_id", &event_id)) {

                    const char *msg = json_object_get_string(body);
                    const char *from = json_object_get_string(sender);
                    const char *eid  = json_object_get_string(event_id);

                    if (strcmp(from, username) != 0) {
                        printf("[+] Dapat pesan: %s\n", msg);

                        /* Contoh respons: jika pesan mengandung "ping" atau "pong" */
                        if (strstr(msg, "ping") != NULL) {
                            WINEMATRIX_send_message(h, room_id, "pong");
                        } else if (strstr(msg, "pong") != NULL) {
                            WINEMATRIX_send_message(h, room_id, "ping");
                        }

                        /* Jika pesan mengandung kata kunci tertentu, kirim reaction */
                        if (strstr(msg, "archana") != NULL || strstr(msg, "berry") != NULL) {
                            send_reaction(h, room_id, eid, "🫐");
                        }
                    }
                }
            }
        }

        json_object_put(root);
        usleep(100000);
    }
}

int main(void) {
    const char *config_filename = "config.json";

    if (WINEMATRIX_global_init() != 0)
        return -1;

    /* Muat konfigurasi */
    Config *cfg = load_config(config_filename);
    if (!cfg) {
        fprintf(stderr, "Gagal memuat konfigurasi\n");
        WINEMATRIX_global_cleanup();
        return -1;
    }

    WINEMATRIX_handle *handle = NULL;
    /* Jika access_token sudah ada dan tidak kosong, gunakan token tersebut */
    if (cfg->access_token && strlen(cfg->access_token) > 0) {
        handle = malloc(sizeof(WINEMATRIX_handle));
        handle->homeserver   = strdup(cfg->homeserver);
        handle->username     = strdup(cfg->username);
        handle->password     = strdup(cfg->password);
        handle->access_token = strdup(cfg->access_token);
        printf("[+] Menggunakan access token yang sudah ada.\n");
    } else {
        /* Lakukan login dan simpan access token baru ke konfigurasi */
        handle = WINEMATRIX_create(cfg->homeserver, cfg->username, cfg->password);
        if (!handle) {
            fprintf(stderr, "[-] Login gagal.\n");
            free_config(cfg);
            WINEMATRIX_global_cleanup();
            return -1;
        }
        /* Update konfigurasi dengan token baru */
        free(cfg->access_token);
        cfg->access_token = strdup(handle->access_token);
        if (save_config(config_filename, cfg) != 0)
            fprintf(stderr, "Peringatan: Gagal menyimpan konfigurasi.\n");
    }

    printf("[+] Login sukses. Access token: %s\n", handle->access_token);
    /* Join ke room */
    if (WINEMATRIX_join_room(handle, cfg->room_id) != 0)
        fprintf(stderr, "[-] Gagal join room %s\n", cfg->room_id);

    /* Mulai loop untuk mendengarkan dan merespon pesan */
    listen_and_respond(handle, cfg->room_id, cfg->username);

    /* Sebelum keluar, hapus access token pada file konfigurasi */
    free(cfg->access_token);
    cfg->access_token = strdup("");
    if (save_config(config_filename, cfg) != 0)
        fprintf(stderr, "Peringatan: Gagal menghapus access token dari konfigurasi.\n");

    WINEMATRIX_free(handle);
    free_config(cfg);
    WINEMATRIX_global_cleanup();
    return 0;
}