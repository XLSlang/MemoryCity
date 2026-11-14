#ifndef TERRAIN_H
#define TERRAIN_H

#include <stdint.h>

// 地形形状/特性：决定 tile 类型和生成倾向
typedef enum {
    TERRAIN_SHAPE_FLAT = 0,     // 平坦草地，适合建造
    TERRAIN_SHAPE_FOREST,       // 树林密集
    TERRAIN_SHAPE_ROCKY,        // 石头/崎岖
    TERRAIN_SHAPE_WATER,        // 水域（暂用 grass 占位，后续扩展）
    TERRAIN_SHAPE_COUNT
} TerrainShape;

typedef struct {
    int min_elev;       // 该形状的最低噪声值
    int max_elev;       // 最高噪声值
    float tree_prob;    // 在该形状内生成树的概率
    float grass_prob;   // 草地概率
} TerrainProfile;

// 世界级参数
typedef struct {
    uint32_t seed;
    int chunk_size;     // 每个 chunk 的边长
} WorldGen;

// 初始化全局世界参数
void terrain_world_init(uint32_t seed, int chunk_size);

// 确定性哈希：同一 (wx, wy) 永远返回同一值
uint32_t terrain_hash2d(int wx, int wy);

// 获取世界坐标处的地形形状
TerrainShape terrain_shape_at(int wx, int wy);

// 获取世界坐标处的地形高度（0~255）
int terrain_elev_at(int wx, int wy);

// 根据形状和坐标决定基础 tile（0=grass, 2=tree, 3=rock 待扩展）
int terrain_base_tile(int wx, int wy);

#endif // TERRAIN_H