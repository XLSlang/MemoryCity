#ifndef ITEM_H
#define ITEM_H

#include <stdbool.h>

#define ITEM_MAX_W 16
#define ITEM_MAX_H 16

typedef struct {
    char id[32];
    char name[64];
    int  width;
    int  height;
    char pixels[ITEM_MAX_H][ITEM_MAX_W + 1];  // 每行字符串，'.' 表示透明

    char tile[16];   // 字符串，运行时由 game_tile_from_name 映射到 TileType
    int  cost;       // 建造花费
    char sprite[32]; // 可选，引用 assets/sprites/<sprite>.json
} ItemDef;

void  item_init(void);                       // 扫描 assets/items/
int   item_count(void);
const ItemDef* item_get(int idx);
const ItemDef* item_find(const char *id);

// 在 cairo 上下文中绘制（每个字符一个方块，scale 为像素大小）
void  item_render(void *cr, const ItemDef *it, double x, double y, double scale);

#endif