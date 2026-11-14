#include "chunk.h"
#include "terrain.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HASH_BUCKETS 256

static Chunk* g_buckets[HASH_BUCKETS];
static int    g_chunk_count = 0;

// 世界坐标 -> chunk 坐标（支持负数，向下取整）
static void world_to_chunk(int wx, int wy, int *cx, int *cy, int *lx, int *ly) {
    *cx = (wx >= 0) ? (wx / CHUNK_SIZE) : ((wx - CHUNK_SIZE + 1) / CHUNK_SIZE);
    *cy = (wy >= 0) ? (wy / CHUNK_SIZE) : ((wy - CHUNK_SIZE + 1) / CHUNK_SIZE);
    *lx = wx - (*cx) * CHUNK_SIZE;
    *ly = wy - (*cy) * CHUNK_SIZE;
}

static uint32_t chunk_hash(int cx, int cy) {
    uint32_t h = (uint32_t)(cx * 73856093) ^ (uint32_t)(cy * 19349663);
    return h % HASH_BUCKETS;
}

void chunk_world_init(uint32_t seed) {
    chunk_world_free();
    terrain_world_init(seed, CHUNK_SIZE);
}

static Chunk* chunk_create(int cx, int cy) {
    Chunk *c = (Chunk*)calloc(1, sizeof(Chunk));
    if (!c) return NULL;
    c->cx = cx;
    c->cy = cy;
    c->base_ready = false;
    return c;
}

static void chunk_generate_base(Chunk *c) {
    if (c->base_ready) return;
    for (int ly = 0; ly < CHUNK_SIZE; ly++) {
        for (int lx = 0; lx < CHUNK_SIZE; lx++) {
            int wx = c->cx * CHUNK_SIZE + lx;
            int wy = c->cy * CHUNK_SIZE + ly;
            c->base_tile[ly][lx] = terrain_base_tile(wx, wy);
        }
    }
    c->base_ready = true;
}

Chunk* chunk_get(int cx, int cy) {
    uint32_t b = chunk_hash(cx, cy);
    Chunk *c = g_buckets[b];
    while (c) {
        if (c->cx == cx && c->cy == cy) return c;
        c = c->next;
    }

    c = chunk_create(cx, cy);
    if (!c) return NULL;
    chunk_generate_base(c);
    c->next = g_buckets[b];
    g_buckets[b] = c;
    g_chunk_count++;
    return c;
}

int chunk_get_tile(int wx, int wy) {
    int cx, cy, lx, ly;
    world_to_chunk(wx, wy, &cx, &cy, &lx, &ly);
    Chunk *c = chunk_get(cx, cy);
    if (!c) return 0;
    if (c->has_overlay[ly][lx]) {
        // overlay 存的是 tile 枚举值，这里约定：
        // 0=grass, 1=road, 2=house, 3=tree（与 game.h TileType 对齐）
        // 我们用 base_tile 的扩展：overlay 时另存 tile 值到 base_tile 高位不可行，
        // 所以用一个约定：grid_id 非空即 overlay，tile 由 id 反查太麻烦。
        // 简化：overlay 的 tile 值直接写回 base_tile 缓存。
        return c->base_tile[ly][lx];
    }
    return c->base_tile[ly][lx];
}

const char* chunk_get_id(int wx, int wy) {
    static char empty[1] = "";
    int cx, cy, lx, ly;
    world_to_chunk(wx, wy, &cx, &cy, &lx, &ly);
    Chunk *c = chunk_get(cx, cy);
    if (!c || !c->has_overlay[ly][lx]) return empty;
    return c->grid_id[ly][lx];
}

void chunk_set_overlay(int wx, int wy, int tile, const char *id) {
    int cx, cy, lx, ly;
    world_to_chunk(wx, wy, &cx, &cy, &lx, &ly);
    Chunk *c = chunk_get(cx, cy);
    if (!c) return;
    c->has_overlay[ly][lx] = 1;
    c->base_tile[ly][lx] = tile;
    if (id && id[0]) {
        snprintf(c->grid_id[ly][lx], GRID_ID_LEN, "%s", id);
    } else {
        c->grid_id[ly][lx][0] = '\0';
    }
}

void chunk_clear_overlay(int wx, int wy) {
    int cx, cy, lx, ly;
    world_to_chunk(wx, wy, &cx, &cy, &lx, &ly);
    Chunk *c = chunk_get(cx, cy);
    if (!c) return;
    c->has_overlay[ly][lx] = 0;
    c->grid_id[ly][lx][0] = '\0';
    // 恢复基础地形
    c->base_tile[ly][lx] = terrain_base_tile(wx, wy);
}

int chunk_count(void) { return g_chunk_count; }

void chunk_world_free(void) {
    for (int i = 0; i < HASH_BUCKETS; i++) {
        Chunk *c = g_buckets[i];
        while (c) {
            Chunk *next = c->next;
            free(c);
            c = next;
        }
        g_buckets[i] = NULL;
    }
    g_chunk_count = 0;
}

typedef void (*ChunkVisitor)(Chunk *c, void *user);

void chunk_foreach(ChunkVisitor fn, void *user) {
    if (!fn) return;
    for (int i = 0; i < HASH_BUCKETS; i++) {
        Chunk *c = g_buckets[i];
        while (c) {
            fn(c, user);
            c = c->next;
        }
    }
}