#include "dsl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* dsl_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

void dsl_trim(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) s[--len] = '\0';
}

bool dsl_split(const char *line, char *key, char *value) {
    if (line[0] != '@') return false;
    const char *colon = strchr(line, ':');
    if (!colon) return false;

    size_t klen = (size_t)(colon - line - 1);
    if (klen >= 64) klen = 63;
    memcpy(key, line + 1, klen);
    key[klen] = '\0';
    dsl_trim(key);

    const char *v = colon + 1;
    while (*v && isspace((unsigned char)*v)) v++;
    size_t vlen = strlen(v);
    if (vlen >= 512) vlen = 511;
    memcpy(value, v, vlen);
    value[vlen] = '\0';
    dsl_trim(value);
    return true;
}

void dsl_parse(const char *text, DslCallback cb, void *user) {
    if (!text) return;
    char line[1024];
    const char *p = text;
    while (*p) {
        size_t i = 0;
        bool overflow = false;
        while (*p && *p != '\n') {
            if (i < sizeof(line) - 1) line[i++] = *p;
            else overflow = true;       // 超长行：丢弃多余字符
            p++;
        }
        line[i] = '\0';
        if (*p == '\n') p++;

        if (overflow) continue;         // 超长行直接跳过

        char *trimmed = line;
        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == '#') continue;

        char key[64], value[512];
        if (dsl_split(trimmed, key, value)) {
            cb(key, value, user);
        }
    }
}