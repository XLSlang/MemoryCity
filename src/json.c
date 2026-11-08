#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static void skip_ws(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static JsonValue* new_value(JsonType t) {
    JsonValue *v = (JsonValue*)calloc(1, sizeof(JsonValue));
    if (v) v->type = t;
    return v;
}

static char* parse_string(const char **p) {
    if (**p != '"') return NULL;
    (*p)++;
    size_t cap = 32, len = 0;
    char *buf = (char*)malloc(cap);
    if (!buf) return NULL;
    while (**p && **p != '"') {
        char c = **p;
        if (c == '\\' && (*p)[1]) {
            (*p)++;
            char e = **p;
            switch (e) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '"': c = '"';  break;
                case '\\': c = '\\';break;
                case '/': c = '/';  break;
                default: c = e;     break;
            }
        }
        if (len + 2 > cap) {
            cap *= 2;
            char *nb = (char*)realloc(buf, cap);
            if (!nb) { free(buf); return NULL; }
            buf = nb;
        }
        buf[len++] = c;
        (*p)++;
    }
    if (**p != '"') { free(buf); return NULL; }
    (*p)++;
    buf[len] = '\0';
    return buf;
}

static JsonValue* parse_value(const char **p, char *err, int errsz);

static JsonValue* parse_object(const char **p, char *err, int errsz) {
    JsonValue *v = new_value(JSON_OBJECT);
    if (!v) { snprintf(err, errsz, "内存不足"); return NULL; }
    (*p)++;
    skip_ws(p);
    JsonMember **tail = &v->as.object.head;
    while (**p && **p != '}') {
        skip_ws(p);
        if (**p != '"') { snprintf(err, errsz, "期望对象键字符串"); json_free(v); return NULL; }
        char *key = parse_string(p);
        if (!key) { snprintf(err, errsz, "解析对象键失败"); json_free(v); return NULL; }
        skip_ws(p);
        if (**p != ':') { snprintf(err, errsz, "期望 ':'"); free(key); json_free(v); return NULL; }
        (*p)++;
        JsonValue *val = parse_value(p, err, errsz);
        if (!val) { free(key); json_free(v); return NULL; }

        JsonMember *m = (JsonMember*)calloc(1, sizeof(JsonMember));
        if (!m) { free(key); json_free(val); json_free(v); snprintf(err, errsz, "内存不足"); return NULL; }
        m->key = key;
        m->value = val;
        *tail = m;
        tail = &m->next;
        v->as.object.count++;

        skip_ws(p);
        if (**p == ',') { (*p)++; skip_ws(p); }
        else break;
    }
    if (**p != '}') { snprintf(err, errsz, "期望 '}'"); json_free(v); return NULL; }
    (*p)++;
    return v;
}

static JsonValue* parse_array(const char **p, char *err, int errsz) {
    JsonValue *v = new_value(JSON_ARRAY);
    if (!v) { snprintf(err, errsz, "内存不足"); return NULL; }
    (*p)++;
    skip_ws(p);
    int cap = 8;
    v->as.array.items = (JsonValue**)calloc((size_t)cap, sizeof(JsonValue*));
    if (!v->as.array.items) { json_free(v); snprintf(err, errsz, "内存不足"); return NULL; }
    while (**p && **p != ']') {
        JsonValue *item = parse_value(p, err, errsz);
        if (!item) { json_free(v); return NULL; }
        if (v->as.array.count + 1 > cap) {
            cap *= 2;
            JsonValue **ni = (JsonValue**)realloc(v->as.array.items, (size_t)cap * sizeof(JsonValue*));
            if (!ni) { json_free(item); json_free(v); snprintf(err, errsz, "内存不足"); return NULL; }
            v->as.array.items = ni;
        }
        v->as.array.items[v->as.array.count++] = item;
        skip_ws(p);
        if (**p == ',') { (*p)++; skip_ws(p); }
        else break;
    }
    if (**p != ']') { snprintf(err, errsz, "期望 ']'"); json_free(v); return NULL; }
    (*p)++;
    return v;
}

static JsonValue* parse_value(const char **p, char *err, int errsz) {
    skip_ws(p);
    char c = **p;
    if (c == '{') return parse_object(p, err, errsz);
    if (c == '[') return parse_array(p, err, errsz);
    if (c == '"') {
        char *s = parse_string(p);
        if (!s) { snprintf(err, errsz, "解析字符串失败"); return NULL; }
        JsonValue *v = new_value(JSON_STRING);
        if (!v) { free(s); snprintf(err, errsz, "内存不足"); return NULL; }
        v->as.str = s;
        return v;
    }
    if (strncmp(*p, "true", 4) == 0)  { (*p) += 4; JsonValue *v = new_value(JSON_BOOL); if (v) v->as.b = true;  return v; }
    if (strncmp(*p, "false", 5) == 0) { (*p) += 5; JsonValue *v = new_value(JSON_BOOL); if (v) v->as.b = false; return v; }
    if (strncmp(*p, "null", 4) == 0)  { (*p) += 4; return new_value(JSON_NULL); }

    char *end = NULL;
    double num = strtod(*p, &end);
    if (end == *p) { snprintf(err, errsz, "无法识别的值"); return NULL; }
    *p = end;
    JsonValue *v = new_value(JSON_NUMBER);
    if (v) v->as.num = num;
    return v;
}

JsonValue* json_parse(const char *text, char *errbuf, int errbuf_size) {
    if (!text) return NULL;
    char local_err[128] = {0};
    if (!errbuf) { errbuf = local_err; errbuf_size = sizeof(local_err); }
    errbuf[0] = '\0';
    const char *p = text;
    JsonValue *v = parse_value(&p, errbuf, errbuf_size);
    return v;
}

void json_free(JsonValue *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING: free(v->as.str); break;
        case JSON_ARRAY:
            for (int i = 0; i < v->as.array.count; i++) json_free(v->as.array.items[i]);
            free(v->as.array.items);
            break;
        case JSON_OBJECT: {
            JsonMember *m = v->as.object.head;
            while (m) {
                JsonMember *next = m->next;
                free(m->key);
                json_free(m->value);
                free(m);
                m = next;
            }
            break;
        }
        default: break;
    }
    free(v);
}

JsonValue* json_object_get(JsonValue *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT) return NULL;
    JsonMember *m = obj->as.object.head;
    while (m) {
        if (strcmp(m->key, key) == 0) return m->value;
        m = m->next;
    }
    return NULL;
}

JsonValue* json_array_get(JsonValue *arr, int idx) {
    if (!arr || arr->type != JSON_ARRAY) return NULL;
    if (idx < 0 || idx >= arr->as.array.count) return NULL;
    return arr->as.array.items[idx];
}

int json_array_size(JsonValue *arr) {
    if (!arr || arr->type != JSON_ARRAY) return 0;
    return arr->as.array.count;
}

const char* json_get_string(JsonValue *v, const char *fallback) {
    if (!v || v->type != JSON_STRING) return fallback;
    return v->as.str;
}

double json_get_number(JsonValue *v, double fallback) {
    if (!v || v->type != JSON_NUMBER) return fallback;
    return v->as.num;
}

bool json_get_bool(JsonValue *v, bool fallback) {
    if (!v || v->type != JSON_BOOL) return fallback;
    return v->as.b;
}