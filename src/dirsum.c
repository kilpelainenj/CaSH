#define _XOPEN_SOURCE 700
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "dirsum.h"
#include <curl/curl.h>

#ifndef DIRSUM_MAX_FILES
#define DIRSUM_MAX_FILES 120 // hard cap of files to sample
#endif
#ifndef DIRSUM_BYTES_PER_FILE
#define DIRSUM_BYTES_PER_FILE 2048 //  read first 2048 bytes per file
#endif

/* A string buffer */
typedef struct {
    char* data;
    size_t len;
} sbuf;

static void sbuf_init(sbuf* b)
{
    b->data = NULL;
    b->len = 0;
}
static void sbuf_free(sbuf* b)
{
    free(b->data);
    b->data = NULL;
    b->len = 0;
}
static void sbuf_append(sbuf* b, const char* p, size_t n)
{
    char* np = realloc(b->data, b->len + n + 1);
    if (!np)
        return;
    memcpy(np + b->len, p, n);
    b->len += n;
    np[b->len] = '\0';
    b->data = np;
}

/* Replace invalid UTF-8 sequences with '?'. Keep LF/CR; other controls => space. */
static char* sanitize_utf8(const unsigned char* src, size_t n)
{
    if (!src)
        return NULL;
    char* out = malloc(n + 1);
    if (!out)
        return NULL;
    const unsigned char *p = src, *end = src + n;
    char* w = out;

    while (p < end) {
        unsigned char c = *p;

        if (c < 0x80) { // ASCII
            if (c == '\n' || c == '\r')
                *w++ = (char)c;
            else if (c < 0x20 || c == 0x7f)
                *w++ = ' ';
            else
                *w++ = (char)c;
            p++;
            continue;
        }

        size_t need = 0;
        unsigned char c1 = 0, c2 = 0, c3 = 0;
        if ((c & 0xE0) == 0xC0) { // 2-byte
            if (end - p >= 2) {
                c1 = p[1];
                if (c >= 0xC2 && c <= 0xDF && (c1 & 0xC0) == 0x80)
                    need = 2;
            }
        } else if ((c & 0xF0) == 0xE0) { // 3-byte
            if (end - p >= 3) {
                c1 = p[1];
                c2 = p[2];
                if (((c == 0xE0 && c1 >= 0xA0 && c1 <= 0xBF) ||
                     (c >= 0xE1 && c <= 0xEC && (c1 & 0xC0) == 0x80) ||
                     (c == 0xED && c1 >= 0x80 && c1 <= 0x9F) ||
                     (c >= 0xEE && c <= 0xEF && (c1 & 0xC0) == 0x80)) &&
                    ((c2 & 0xC0) == 0x80))
                    need = 3;
            }
        } else if ((c & 0xF8) == 0xF0) { // 4-byte
            if (end - p >= 4) {
                c1 = p[1];
                c2 = p[2];
                c3 = p[3];
                if (((c == 0xF0 && c1 >= 0x90 && c1 <= 0xBF) ||
                     (c >= 0xF1 && c <= 0xF3 && (c1 & 0xC0) == 0x80) ||
                     (c == 0xF4 && c1 >= 0x80 && c1 <= 0x8F)) &&
                    ((c2 & 0xC0) == 0x80) && ((c3 & 0xC0) == 0x80))
                    need = 4;
            }
        }
        if (need) {
            for (size_t i = 0; i < need; i++)
                *w++ = (char)p[i];
            p += need;
        } else {
            *w++ = '?';
            p++;
        }
    }
    *w = '\0';
    return out;
}

// helpers

static int looks_text(const unsigned char* buf, size_t n)
{
    size_t non = 0;
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == 0)
            return 0;
        if (!(isprint(buf[i]) || isspace(buf[i])))
            non++;
    }
    return (non * 100 / (n ? n : 1)) < 20;
}

static int is_hidden(const char* name) { return name[0] == '.'; }

static int should_skip_dir(const char* name)
{
    return (!strcmp(name, ".git") || !strcmp(name, ".hg") || !strcmp(name, ".svn") ||
            !strcmp(name, "build") || !strcmp(name, "dist") || !strcmp(name, "node_modules"));
}

/* Read first max_bytes; return sanitized UTF-8 string in *out (malloc’d) */
static int read_head(const char* path, char** out, size_t max_bytes)
{
    FILE* f = fopen(path, "rb");
    if (!f)
        return -1;
    unsigned char* buf = malloc(max_bytes);
    if (!buf) {
        fclose(f);
        return -1;
    }
    size_t n = fread(buf, 1, max_bytes, f);
    fclose(f);
    if (n == 0 || !looks_text(buf, n)) {
        free(buf);
        return -2;
    }
    char* san = sanitize_utf8(buf, n);
    free(buf);
    if (!san)
        return -1;
    *out = san;
    return (int)strlen(san);
}

/*  collects JSON objects {path,size_bytes,head} */
static void walk_dir(const char* root, size_t max_bytes, cJSON* files_arr, int* count)
{
    DIR* d = opendir(root);
    if (!d)
        return;
    struct dirent* ent;
    while ((ent = readdir(d))) {
        if (is_hidden(ent->d_name))
            continue;
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", root, ent->d_name);

        struct stat st;
        if (lstat(path, &st) != 0)
            continue;

        if (S_ISDIR(st.st_mode)) {
            if (should_skip_dir(ent->d_name))
                continue;
            walk_dir(path, max_bytes, files_arr, count);
            if (*count >= DIRSUM_MAX_FILES)
                break;
        } else if (S_ISREG(st.st_mode)) {
            if (*count >= DIRSUM_MAX_FILES)
                break;
            char* snippet = NULL;
            int r = read_head(path, &snippet, max_bytes);
            if (r >= 0) {
                cJSON* item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "path", path);
                cJSON_AddNumberToObject(item, "size_bytes", (double)st.st_size);
                cJSON_AddStringToObject(item, "head", snippet);
                cJSON_AddItemToArray(files_arr, item);
                free(snippet);
                (*count)++;
            }
        }
    }
    closedir(d);
}

static char* build_request_json(cJSON* files, const char* model)
{
    /* Payload needs to look like this:
       { "model": "...", "input": [ {role:"system"...},{role:"user",content:"<json>"} ], "stream":
       true } */
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", model);
    cJSON_AddBoolToObject(root, "stream", 1);

    const char* sys =
        "You are a helpful codebase summarizer. Given a list of files with "
        "short text snippets, write a concise overview of what this directory contains. "
        "Group by purpose, note build tools, and suggest the command to build or run if obvious.";

    cJSON* input = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "input", input);

    cJSON* sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    cJSON_AddStringToObject(sys_msg, "content", sys);
    cJSON_AddItemToArray(input, sys_msg);

    cJSON* user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");

    cJSON* payload = cJSON_CreateObject();
    cJSON_AddItemToObject(payload, "files", files);
    char* payload_str = cJSON_PrintUnformatted(payload);
    cJSON_Delete(payload);

    cJSON_AddStringToObject(user_msg, "content", payload_str);
    free(payload_str);
    cJSON_AddItemToArray(input, user_msg);

    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

typedef struct {
    sbuf linebuf;  // accumulates partial chunks until we have full lines
    sbuf blockbuf; // accumulates lines for a single SSE block (until blank line)
} sse_state;

static void sse_state_init(sse_state* st)
{
    sbuf_init(&st->linebuf);
    sbuf_init(&st->blockbuf);
}
static void sse_state_free(sse_state* st)
{
    sbuf_free(&st->linebuf);
    sbuf_free(&st->blockbuf);
}

/* Process one complete SSE block (one or more lines, terminated by a blank line). */
static void sse_process_block(const char* blk, size_t n)
{
    // Extract last 'data:' line (OpenAI sends one JSON object per block).
    const char *p = blk, *end = blk + n;
    const char* data_start = NULL;
    size_t data_len = 0;

    while (p < end) {
        const char* nl = memchr(p, '\n', (size_t)(end - p));
        size_t len = nl ? (size_t)(nl - p) : (size_t)(end - p);
        while (len && p[len - 1] == '\r')
            len--; // trim CR

        if (len >= 6 && !memcmp(p, "data: ", 6)) {
            data_start = p + 6;
            data_len = len - 6;
        }
        p = nl ? nl + 1 : end;
    }
    if (!data_start || data_len == 0)
        return;

    char* json = (char*)malloc(data_len + 1);
    if (!json)
        return;
    memcpy(json, data_start, data_len);
    json[data_len] = '\0';

    cJSON* root = cJSON_Parse(json);
    free(json);
    if (!root)
        return;

    // Use data.type to determine event kind
    cJSON* type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (cJSON_IsString(type) && type->valuestring) {
        const char* t = type->valuestring;

        // Any *.delta event that has a "delta" string: print it incrementally
        if (strstr(t, ".delta") != NULL) {
            cJSON* delta = cJSON_GetObjectItemCaseSensitive(root, "delta");
            if (cJSON_IsString(delta) && delta->valuestring) {
                fputs(delta->valuestring, stdout);
                fflush(stdout);
            }
        }
        if (!strcmp(t, "response.completed")) {
            fputc('\n', stdout);
            fflush(stdout);
        }
        if (!strcmp(t, "error")) {
            cJSON* err = cJSON_GetObjectItemCaseSensitive(root, "error");
            if (err) {
                char* e = cJSON_PrintUnformatted(err);
                if (e) {
                    fprintf(stderr, "\n[stream error] %s\n", e);
                    free(e);
                }
            }
        }
    }
    cJSON_Delete(root);
}

// Process the response from OpenAI API
static size_t sse_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    sse_state* st = (sse_state*)userdata;
    size_t n = size * nmemb;

    // Append and process complete lines
    sbuf_append(&st->linebuf, ptr, n);
    char* buf = st->linebuf.data;

    size_t start = 0;
    for (size_t i = 0; i < st->linebuf.len; ++i) {
        if (buf[i] == '\n') {
            size_t linelen = i - start;

            // in HTTP, lines end in \r\n, so we have to trim \r
            size_t trimmed = linelen;
            while (trimmed && buf[start + trimmed - 1] == '\r')
                trimmed--;

            // Empty line means a block boundary
            if (trimmed == 0) {
                if (st->blockbuf.len > 0) {
                    sse_process_block(st->blockbuf.data, st->blockbuf.len);
                    st->blockbuf.len = 0;
                    if (st->blockbuf.data)
                        st->blockbuf.data[0] = '\0';
                }
            } else {
                sbuf_append(&st->blockbuf, buf + start, trimmed);
                sbuf_append(&st->blockbuf, "\n", 1);
            }
            start = i + 1;
        }
    }

    // Keep leftover partial line (if any)
    if (start > 0) {
        size_t remain = st->linebuf.len - start;
        memmove(st->linebuf.data, st->linebuf.data + start, remain);
        st->linebuf.len = remain;
        st->linebuf.data[remain] = '\0';
    }
    return n;
}

static int http_post_openai_stream(const char* json_payload)
{
    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "dirsum: set OPENAI_API_KEY\n");
        return -1;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "dirsum: curl init failed\n");
        return -1;
    }

    struct curl_slist* hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    hdrs = curl_slist_append(hdrs, "Accept: text/event-stream");
    char auth[512];
    snprintf(auth, sizeof(auth), "Authorization: Bearer %s", api_key);
    hdrs = curl_slist_append(hdrs, auth);

    sse_state st;
    sse_state_init(&st);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/responses");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sse_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &st);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    sse_state_free(&st);

    if (rc != CURLE_OK) {
        fprintf(stderr, "dirsum: curl error: %s\n", curl_easy_strerror(rc));
        return -1;
    }
    if (code >= 400) {
        fprintf(stderr, "dirsum: HTTP %ld (stream)\n", code);
        return -1;
    }
    return 0;
}

// The main function of the command
int do_dirsum(int argc, char** argv)
{
    const char* root = (argc > 1) ? argv[1] : ".";
    int max_files = DIRSUM_MAX_FILES;
    size_t head_bytes = DIRSUM_BYTES_PER_FILE;
    const char* model = "gpt-4o-mini";
    int debug = 0;

    // flags: dirsum [--debug] [-n max_files] [-b bytes] [-m model] [path]
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--debug")) {
            debug = 1;
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            max_files = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-b") && i + 1 < argc) {
            head_bytes = (size_t)atol(argv[++i]);
        } else if (!strcmp(argv[i], "-m") && i + 1 < argc) {
            model = argv[++i];
        } else if (argv[i][0] != '-') {
            root = argv[i];
        }
    }

    cJSON* files = cJSON_CreateArray();
    int count = 0;
    walk_dir(root, head_bytes, files, &count);

    if (debug) {
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd)))
            strcpy(cwd, "(getcwd failed)");
        fprintf(stderr, "dirsum debug: cwd='%s' root='%s' collected=%d files (bytes/head=%zu)\n",
                cwd, root, count, head_bytes);
    }

    if (count == 0) {
        if (!debug)
            fprintf(stderr, "dirsum: no text-like files found under '%s'\n", root);
        cJSON_Delete(files);
        return 0;
    }

    cJSON* trimmed = cJSON_CreateArray();
    int N = cJSON_GetArraySize(files);
    for (int i = 0; i < N && i < max_files; ++i)
        cJSON_AddItemToArray(trimmed, cJSON_DetachItemFromArray(files, 0));
    cJSON_Delete(files);

    char* req = build_request_json(trimmed, model);
    int rc = http_post_openai_stream(req);
    free(req);
    cJSON_Delete(trimmed);
    return rc ? 1 : 0;
}