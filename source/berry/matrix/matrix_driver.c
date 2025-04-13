#include "matrix_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>

/* Format URL untuk berbagai operasi Matrix */
#define LOGIN_URL_FORMAT "%s/_matrix/client/r0/login"
#define JOIN_URL_FORMAT  "%s/_matrix/client/r0/join/%s?access_token=%s"
#define SEND_URL_FORMAT  "%s/_matrix/client/r0/rooms/%s/send/m.room.message/%ld?access_token=%s"
#define STATE_PIN_URL_FORMAT "%s/_matrix/client/r0/rooms/%s/state/m.room.pinned_events?access_token=%s"
#define REDACT_URL_FORMAT "%s/_matrix/client/r0/rooms/%s/redact/%s/%ld?access_token=%s"

/* Struktur untuk menampung respons dari libcurl */
struct MemoryStruct {
    char *memory;
    size_t size;
};

/**
 * @brief Callback untuk menulis data yang diterima oleh libcurl ke memori.
 *
 * @param contents Pointer ke data yang diterima.
 * @param size Ukuran tiap elemen.
 * @param nmemb Jumlah elemen.
 * @param userp Pointer ke struktur MemoryStruct.
 * @return size_t Jumlah byte yang diproses.
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        /* Gagal alokasi memori */
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

/* Inisialisasi global untuk libcurl */
WINEMATRIXcode
int WINEMATRIX_global_init(void) {
    if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
        fprintf(stderr, "Gagal inisialisasi curl\n");
        return -1;
    }
    return 0;
}

/* Membersihkan resource global libcurl */
WINEMATRIXcode
void WINEMATRIX_global_cleanup(void) {
    curl_global_cleanup();
}

/**
 * @brief Fungsi helper untuk melakukan HTTP request dengan libcurl.
 *
 * @param url URL tujuan request.
 * @param json_data Data JSON (jika ada) yang akan dikirim.
 * @param http_method Metode HTTP ("POST" atau "PUT").
 * @param chunk Pointer ke struktur MemoryStruct untuk menyimpan respons.
 * @return int 0 jika berhasil, -1 jika terjadi kesalahan.
 */
static int perform_http_request(const char *url, const char *json_data, const char *http_method, struct MemoryStruct *chunk)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Gagal inisialisasi curl handle\n");
        return -1;
    }
    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    if (strcmp(http_method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (strcmp(http_method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    }
    
    if (json_data != NULL) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() error: %s\n", curl_easy_strerror(res));
        return -1;
    }
    return 0;
}

/**
 * @brief Fungsi sederhana untuk mengekstrak access_token dari respons JSON.
 *
 * Parsing dilakukan secara sederhana dengan mencari key "access_token".
 *
 * @param response Respons JSON dari login.
 * @return char* Token yang dialokasikan secara dinamis, atau NULL jika tidak ditemukan.
 */
static char* parse_access_token(const char* response)
{
    const char *key = "\"access_token\":\"";
    char *start = strstr(response, key);
    if (!start)
        return NULL;
    start += strlen(key);
    char *end = strchr(start, '\"');
    if (!end)
        return NULL;
    size_t token_len = end - start;
    char *token = malloc(token_len + 1);
    if (!token)
        return NULL;
    strncpy(token, start, token_len);
    token[token_len] = '\0';
    return token;
}

/* Membuat handle baru dan melakukan login ke Matrix */
WINEMATRIXcode
WINEMATRIX_handle* WINEMATRIX_create(const char* homeserver, const char* username, const char* password)
{
    WINEMATRIX_handle *handle = malloc(sizeof(WINEMATRIX_handle));
    if (!handle)
        return NULL;
    handle->homeserver = strdup(homeserver);
    handle->username = strdup(username);
    handle->password = strdup(password);
    handle->access_token = NULL;
    
    /* Buat URL login */
    size_t url_len = strlen(homeserver) + 100;
    char *login_url = malloc(url_len);
    snprintf(login_url, url_len, LOGIN_URL_FORMAT, homeserver);
    
    /* Siapkan data JSON untuk login */
    size_t json_len = strlen(username) + strlen(password) + 150;
    char *json_data = malloc(json_len);
    snprintf(json_data, json_len,
             "{ \"type\": \"m.login.password\", \"user\": \"%s\", \"password\": \"%s\" }",
             username, password);
    
    /* Siapkan struktur untuk menampung respons */
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    if (perform_http_request(login_url, json_data, "POST", &chunk) != 0) {
        free(login_url);
        free(json_data);
        free(chunk.memory);
        WINEMATRIX_free(handle);
        return NULL;
    }
    
    /* Parse access token dari respons login */
    handle->access_token = parse_access_token(chunk.memory);
    if (!handle->access_token) {
        fprintf(stderr, "Gagal mengambil access token dari respons login\n");
        free(login_url);
        free(json_data);
        free(chunk.memory);
        WINEMATRIX_free(handle);
        return NULL;
    }
    
    free(login_url);
    free(json_data);
    free(chunk.memory);
    return handle;
}

/* Bergabung ke room Matrix */
WINEMATRIXcode
int WINEMATRIX_join_room(WINEMATRIX_handle* handle, const char* room_id)
{
    if (!handle || !handle->access_token)
        return -1;
    size_t url_len = strlen(handle->homeserver) + strlen(room_id) + strlen(handle->access_token) + 100;
    char *join_url = malloc(url_len);
    snprintf(join_url, url_len, JOIN_URL_FORMAT, handle->homeserver, room_id, handle->access_token);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    if (perform_http_request(join_url, "{}", "POST", &chunk) != 0) {
        free(join_url);
        free(chunk.memory);
        return -1;
    }
    
    if (strstr(chunk.memory, "\"errcode\"")) {
        fprintf(stderr, "Gagal join room. Respons: %s\n", chunk.memory);
        free(join_url);
        free(chunk.memory);
        return -1;
    }
    
    free(join_url);
    free(chunk.memory);
    return 0;
}

/* Mengirim pesan ke room Matrix */
WINEMATRIXcode
int WINEMATRIX_send_message(WINEMATRIX_handle* handle, const char* room_id, const char* message)
{
    if (!handle || !handle->access_token)
        return -1;
    long txn_id = (long)time(NULL);
    size_t url_len = strlen(handle->homeserver) + strlen(room_id) + strlen(handle->access_token) + 100;
    char *send_url = malloc(url_len);
    snprintf(send_url, url_len, SEND_URL_FORMAT, handle->homeserver, room_id, txn_id, handle->access_token);
    
    size_t json_len = strlen(message) + 150;
    char *json_data = malloc(json_len);
    snprintf(json_data, json_len,
             "{ \"msgtype\": \"m.text\", \"body\": \"%s\" }",
             message);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    if (perform_http_request(send_url, json_data, "PUT", &chunk) != 0) {
        free(send_url);
        free(json_data);
        free(chunk.memory);
        return -1;
    }
    
    printf("Respons pengiriman: %s\n", chunk.memory);
    
    free(send_url);
    free(json_data);
    free(chunk.memory);
    return 0;
}

/* Mengirim pesan reply dengan mengutip pesan asli */
WINEMATRIXcode
int WINEMATRIX_send_reply(WINEMATRIX_handle* handle, const char* room_id, const char* original_event_id,
                            const char* original_message, const char* reply_message)
{
    if (!handle || !handle->access_token)
        return -1;
        
    long txn_id = (long)time(NULL);
    size_t url_len = strlen(handle->homeserver) + strlen(room_id) + strlen(handle->access_token) + 100;
    char *send_url = malloc(url_len);
    snprintf(send_url, url_len, SEND_URL_FORMAT, handle->homeserver, room_id, txn_id, handle->access_token);
    
    /* Format body dengan mengutip pesan asli (prefiks "> ") dan menyertakan balasan */
    size_t body_len = strlen(original_message) + strlen(reply_message) + 200;
    char *body = malloc(body_len);
    snprintf(body, body_len,
             "> %s\n\n%s",
             original_message, reply_message);
    
    size_t json_len = body_len + strlen(original_event_id) + 150;
    char *json_data = malloc(json_len);
    snprintf(json_data, json_len,
             "{ \"msgtype\": \"m.text\", \"body\": \"%s\", "
             "\"m.relates_to\": { \"m.in_reply_to\": { \"event_id\": \"%s\" } } }",
             body, original_event_id);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    int ret = perform_http_request(send_url, json_data, "PUT", &chunk);
    
    printf("Respons reply: %s\n", chunk.memory);
    
    free(send_url);
    free(json_data);
    free(body);
    free(chunk.memory);
    return ret;
}

/* Mengirim pesan direct message (DM) ke room DM yang sudah ada */
WINEMATRIXcode
int WINEMATRIX_send_direct_message(WINEMATRIX_handle* handle, const char* dm_room_id, const char* message)
{
    /* Sama dengan mengirim pesan ke room biasa */
    return WINEMATRIX_send_message(handle, dm_room_id, message);
}

/* Mengirim reaction terhadap pesan tertentu */
WINEMATRIXcode
int WINEMATRIX_send_reaction(WINEMATRIX_handle* handle, const char* room_id, const char* target_event_id, const char* reaction)
{
    if (!handle || !handle->access_token)
        return -1;
    long txn_id = (long)time(NULL);
    
    /* Endpoint untuk reaction dengan tipe event m.reaction */
    size_t url_len = strlen(handle->homeserver) + strlen(room_id) + strlen(handle->access_token) + 100;
    char *send_url = malloc(url_len);
    snprintf(send_url, url_len,
             "%s/_matrix/client/r0/rooms/%s/send/m.reaction/%ld?access_token=%s",
             handle->homeserver, room_id, txn_id, handle->access_token);
    
    size_t json_len = strlen(target_event_id) + strlen(reaction) + 150;
    char *json_data = malloc(json_len);
    snprintf(json_data, json_len,
             "{ \"m.relates_to\": { \"rel_type\": \"m.annotation\", \"event_id\": \"%s\", \"key\": \"%s\" } }",
             target_event_id, reaction);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    int ret = perform_http_request(send_url, json_data, "PUT", &chunk);
    
    printf("Respons reaction: %s\n", chunk.memory);
    
    free(send_url);
    free(json_data);
    free(chunk.memory);
    return ret;
}

/* Menyematkan (pin) pesan di room Matrix */
WINEMATRIXcode
int WINEMATRIX_pin_message(WINEMATRIX_handle* handle, const char* room_id, const char* event_id)
{
    if (!handle || !handle->access_token)
        return -1;
    size_t url_len = strlen(handle->homeserver) + strlen(room_id) + strlen(handle->access_token) + 100;
    char *pin_url = malloc(url_len);
    snprintf(pin_url, url_len, STATE_PIN_URL_FORMAT, handle->homeserver, room_id, handle->access_token);
    
    size_t json_len = strlen(event_id) + 100;
    char *json_data = malloc(json_len);
    snprintf(json_data, json_len, "{ \"pinned\": [ \"%s\" ] }", event_id);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    int ret = perform_http_request(pin_url, json_data, "PUT", &chunk);
    
    printf("Respons pin: %s\n", chunk.memory);
    
    free(pin_url);
    free(json_data);
    free(chunk.memory);
    return ret;
}

/* Menghapus (redact) pesan dari room Matrix */
WINEMATRIXcode
int WINEMATRIX_redact_message(WINEMATRIX_handle* handle, const char* room_id, const char* event_id, const char* reason)
{
    if (!handle || !handle->access_token)
        return -1;
    long txn_id = (long)time(NULL);
    size_t url_len = strlen(handle->homeserver) + strlen(room_id) + strlen(event_id) + strlen(handle->access_token) + 150;
    char *redact_url = malloc(url_len);
    snprintf(redact_url, url_len, REDACT_URL_FORMAT, handle->homeserver, room_id, event_id, txn_id, handle->access_token);
    
    size_t json_len = strlen(reason) + 100;
    char *json_data = malloc(json_len);
    snprintf(json_data, json_len, "{ \"reason\": \"%s\" }", reason);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    int ret = perform_http_request(redact_url, json_data, "POST", &chunk);
    
    printf("Respons redact: %s\n", chunk.memory);
    
    free(redact_url);
    free(json_data);
    free(chunk.memory);
    return ret;
}

/* Forward (bagikan) pesan ke room lain dengan menyertakan kutipan isi pesan asli */
WINEMATRIXcode
int WINEMATRIX_forward_message(WINEMATRIX_handle* handle,
                               const char* dest_room_id,
                               const char* original_room_id,
                               const char* original_event_id,
                               const char* original_message)
{
    if (!handle || !handle->access_token)
        return -1;
    long txn_id = (long)time(NULL);
    size_t url_len = strlen(handle->homeserver) + strlen(dest_room_id) + strlen(handle->access_token) + 100;
    char *send_url = malloc(url_len);
    snprintf(send_url, url_len, SEND_URL_FORMAT, handle->homeserver, dest_room_id, txn_id, handle->access_token);
    
    /* Format pesan forward dengan menyertakan informasi room dan event asli */
    size_t body_len = strlen(original_room_id) + strlen(original_message) + 200;
    char *body = malloc(body_len);
    snprintf(body, body_len,
             "Forwarded message from room %s:\n> (Event: %s)\n> %s",
             original_room_id, original_event_id, original_message);
    
    size_t json_len = body_len + 150;
    char *json_data = malloc(json_len);
    snprintf(json_data, json_len,
             "{ \"msgtype\": \"m.text\", \"body\": \"%s\" }",
             body);
    
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    int ret = perform_http_request(send_url, json_data, "PUT", &chunk);
    
    printf("Respons forward: %s\n", chunk.memory);
    
    free(send_url);
    free(json_data);
    free(body);
    free(chunk.memory);
    return ret;
}

/* Membebaskan memori yang digunakan oleh handle */
WINEMATRIXcode
void WINEMATRIX_free(WINEMATRIX_handle* handle)
{
    if (!handle)
        return;
    free(handle->homeserver);
    free(handle->username);
    free(handle->password);
    if (handle->access_token)
        free(handle->access_token);
    free(handle);
}