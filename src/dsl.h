#ifndef DSL_H
#define DSL_H

#include <stdbool.h>
#include <stddef.h>

// 极简 @key: value 解析结果
typedef struct {
    char key[64];
    char value[512];
} DslKV;

// 读取整个文件到缓冲区（返回 malloc 的 buf，调用方 free；失败返回 NULL）
char* dsl_read_file(const char *path);

// 从文本逐行解析，每遇到 @key: value 就回调一次
typedef void (*DslCallback)(const char *key, const char *value, void *user);
void dsl_parse(const char *text, DslCallback cb, void *user);

// 字符串工具
void dsl_trim(char *s);
bool dsl_split(const char *line, char *key, char *value); // "@k: v" -> key="k", value="v"

#endif