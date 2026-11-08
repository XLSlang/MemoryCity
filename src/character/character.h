#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdbool.h>

// 预留：人物属性系统
typedef struct {
    char id[32];
    char name[64];
    char portrait[128];
    int  honesty;
    int  efficiency;
    int  charisma;
} CharDef;

void character_init(void);
int  character_count(void);
const CharDef* character_get(int idx);
const CharDef* character_find(const char *id);

#endif