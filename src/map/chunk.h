#ifndef CHUNK_H
#define CHUNK_H

#include <stdbool.h>
#include <stdint.h>

#define CHUNK_SIZE 16
#define GRID_ID_LEN 32

// 单个 chunk：基础地形由 seed 确定性生成，overlay 存玩家修改
typedef struct Chunk {
    int cx, cy;

    // overlay 稀疏表：每格是否为玩家修改，以及对应 grid_id
    uint8_t  has_overlay[CHUNK_SIZE][CHUNK_SIZE];
    char     grid_id[CHUNK_SIZE][CHUNK_SIZE][GRID_ID_LEN];

    // 已生成的基础 tile 缓存（0=grass, 1=road, 2=house, 3=tree 之外自定义）
    // 这里用 int 存，避免依赖 game.h 的枚举顺序
    int      base_tile[CHUNK_SIZE][CHUNK_SIZE];
    bool     base_ready;

    struct Chunk *next; // 哈希桶链表
} Chunk;

// 世界初始化：seed + 默认 chunk 大小
void chunk_world_init(uint32_t seed);

// 查找或创建 chunk（按需生成基础地形）
Chunk* chunk_get(int cx, int cy);

// 世界坐标 -> 基础 tile（优先 overlay，其次 chunk 基础）
int chunk_get_tile(int wx, int wy);

// 世界坐标 -> grid_id 字符串（无则返回空串）
const char* chunk_get_id(int wx, int wy);

// 写入 overlay（建造/拆除都走这里）
void chunk_set_overlay(int wx, int wy, int tile, const char *id);

// 清除 overlay，回落到基础地形
void chunk_clear_overlay(int wx, int wy);

// 统计已加载 chunk 数（调试用）
int chunk_count(void);

// 释放所有 chunk
void chunk_world_free(void);

typedef void (*ChunkVisitor)(Chunk *c, void *user);
void chunk_foreach(ChunkVisitor fn, void *user);

#endif // CHUNK_H