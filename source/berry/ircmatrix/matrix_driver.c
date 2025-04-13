#include "matrix_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MatrixHandle {
    char *homeserver;    /* URL homeserver, misalnya "https://matrix.example.com" */
    char *access_token;  /* Diambil saat login */
};

/* Struktur untuk menyimpan response dari libcurl */
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        /* out of memory! */
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

/* Fungsi untuk mengekstrak access_token dari response JSON */
static char* extract_access_token(const char* response) {
    const char *key = "\"access_token\":\"";
    char *start = strstr(response, key);
    if (!start) {
        return NULL;
    }
    start += strlen(key);
    char *end = strchr(start, '"');
    if (!end) {
        return NULL;
    }
    size_t token_len = end - start;
    char *token = malloc(token_len + 1);
    if (!token) {
        return NULL;
    }
    strncpy(token, start, token_len);
    token[token_len] = '\0';
    return token;
}

MatrixHandle* matrix_connect(const ChatConfig* config) {
    MatrixHandle *handle = malloc(sizeof(MatrixHandle));
    if (!handle) {
        return NULL;
    }

    handle->homeserver = strdup(config->server);
    handle->access_token = NULL;
    CURL *curl = curl_easy_init();
    if (!curl) {
        free(handle->homeserver);
        free(handle);
        return NULL;
    }

    char url[256];
    snprintf(url, sizeof(url), "%s/_matrix/client/r0/login", config->server);

    char postfields[256];
    snprintf(postfields, sizeof(postfields), 
             "{\"type\":\"m.login.password\", \"user\":\"%s\", \"password\":\"%s\"}", 
             config->username, config->password);

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  
    chunk.size = 0;
    if (!chunk.memory) {
        curl_easy_cleanup(curl);
        free(handle->homeserver);
        free(handle);
        return NULL;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(chunk.memory);
        free(handle->homeserver);
        free(handle);
        return NULL;
    }

    handle->access_token = extract_access_token(chunk.memory);
    if (!handle->access_token) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(chunk.memory);
        free(handle->homeserver);
        free(handle);
        return NULL;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(chunk.memory);
    return handle;
}

void matrix_send_message(MatrixHandle* handle, const char *room, const char *message) {
    if (!handle || !handle->access_token) {
        return;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return;
    }

    const char *txn_id = "1"; 
    char url[512];
    snprintf(url, sizeof(url), "%s/_matrix/client/r0/rooms/%s/send/m.room.message/%s?access_token=%s", 
             handle->homeserver, room, txn_id, handle->access_token);

    char postfields[512];
    snprintf(postfields, sizeof(postfields), 
             "{\"msgtype\":\"m.text\", \"body\":\"%s\"}", message);

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  
    chunk.size = 0;
    if (!chunk.memory) {
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Matrix send message request failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    free(chunk.memory);
}

void matrix_disconnect(MatrixHandle* handle) {
    if (!handle) return;

    free(handle->homeserver);
    free(handle->access_token);
    free(handle);
}