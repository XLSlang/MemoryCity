#include "terrain.h"
#include <math.h>

static WorldGen g_world = { .seed = 1u, .chunk_size = 16 };

void terrain_world_init(uint32_t seed, int chunk_size) {
    g_world.seed = seed ? seed : 1u;
    g_world.chunk_size = chunk_size > 0 ? chunk_size : 16;
}

// 经典 32 位整数哈希（Wang hash 变体），保证确定性
uint32_t terrain_hash2d(int wx, int wy) {
    uint32_t h = g_world.seed;
    h ^= (uint32_t)(wx * 374761393u);
    h ^= (uint32_t)(wy * 668265263u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return h;
}

// 简单值噪声：在格点哈希间做双线性插值
static float value_noise(float x, float y) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    int x1 = x0 + 1, y1 = y0 + 1;
    float fx = x - (float)x0, fy = y - (float)y0;

    uint32_t h00 = terrain_hash2d(x0, y0);
    uint32_t h10 = terrain_hash2d(x1, y0);
    uint32_t h01 = terrain_hash2d(x0, y1);
    uint32_t h11 = terrain_hash2d(x1, y1);

    float v00 = (h00 & 0xFFFF) / 65535.0f;
    float v10 = (h10 & 0xFFFF) / 65535.0f;
    float v01 = (h01 & 0xFFFF) / 65535.0f;
    float v11 = (h11 & 0xFFFF) / 65535.0f;

    // 平滑插值
    float sx = fx * fx * (3.0f - 2.0f * fx);
    float sy = fy * fy * (3.0f - 2.0f * fy);

    float a = v00 + (v10 - v00) * sx;
    float b = v01 + (v11 - v01) * sx;
    return a + (b - a) * sy;
}

// 多倍频叠加，得到更自然的地形
int terrain_elev_at(int wx, int wy) {
    float scale = 0.08f;   // 地形频率，越小越平缓
    float n = 0.0f;
    float amp = 1.0f;
    float freq = scale;
    for (int i = 0; i < 3; i++) {
        n += value_noise((float)wx * freq, (float)wy * freq) * amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    // 归一化到 0~255
    int e = (int)(n * 255.0f / 1.75f);
    if (e < 0) e = 0;
    if (e > 255) e = 255;
    return e;
}

TerrainShape terrain_shape_at(int wx, int wy) {
    int e = terrain_elev_at(wx, wy);
    if (e < 60)  return TERRAIN_SHAPE_WATER;
    if (e < 110) return TERRAIN_SHAPE_FLAT;
    if (e < 170) return TERRAIN_SHAPE_FOREST;
    return TERRAIN_SHAPE_ROCKY;
}

// 基础 tile：0=grass, 2=tree（跟你 game.h 的 TileType 顺序对齐）
int terrain_base_tile(int wx, int wy) {
    TerrainShape s = terrain_shape_at(wx, wy);
    uint32_t h = terrain_hash2d(wx + 7919, wy - 104729);
    float r = (h & 0xFFFF) / 65535.0f;

    switch (s) {
        case TERRAIN_SHAPE_WATER:
            return 0; // 暂用 grass 占位，后续加 TILE_WATER
        case TERRAIN_SHAPE_FLAT:
            return (r < 0.02f) ? 2 : 0;
        case TERRAIN_SHAPE_FOREST:
            return (r < 0.55f) ? 2 : 0;
        case TERRAIN_SHAPE_ROCKY:
            return (r < 0.70f) ? 2 : 0;
        default:
            return 0;
    }
}