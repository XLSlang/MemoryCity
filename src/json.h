#ifndef JSON_H
#define JSON_H

#include <stdbool.h>

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonValue JsonValue;

typedef struct JsonMember {
    char *key;
    JsonValue *value;
    struct JsonMember *next;
} JsonMember;

struct JsonValue {
    JsonType type;
    union {
        bool      b;
        double    num;
        char     *str;
        struct { JsonValue **items; int count; } array;
        struct { JsonMember *head;   int count; } object;
    } as;
};

// 解析：返回 NULL 表示失败（msg 会带错误信息，可为 NULL）
JsonValue* json_parse(const char *text, char *errbuf, int errbuf_size);
void       json_free(JsonValue *v);

// 取值辅助（找不到返回 NULL）
JsonValue*  json_object_get(JsonValue *obj, const char *key);
JsonValue*  json_array_get(JsonValue *arr, int idx);
int         json_array_size(JsonValue *arr);
const char* json_get_string(JsonValue *v, const char *fallback);
double      json_get_number(JsonValue *v, double fallback);
bool        json_get_bool(JsonValue *v, bool fallback);

#endif