#include "game.h"
#include "map/chunk.h"
#include "save/save.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static GameState g_state;
static uint32_t  g_world_seed = 1;

GameState* game_get_state(void) { return &g_state; }
uint32_t   game_get_world_seed(void) { return g_world_seed; }

TileType game_tile_from_name(const char *name) {
    if (!name) return TILE_GRASS;
    if (strcmp(name, "house") == 0) return TILE_HOUSE;
    if (strcmp(name, "road")  == 0) return TILE_ROAD;
    if (strcmp(name, "tree")  == 0) return TILE_TREE;
    if (strcmp(name, "grass") == 0) return TILE_GRASS;
    return TILE_GRASS;
}

void game_init_world(uint32_t seed) {
    g_world_seed = seed ? seed : 1u;
    chunk_world_init(g_world_seed);
}

void game_init(void) {
    // 默认世界种子：时间种子，后续 game_load_from 会覆盖
    game_init_world((uint32_t)time(NULL));

    g_state.has_hover = false;
    g_state.has_selection = false;
    g_state.cam_offset_x = 0;
    g_state.cam_offset_y = 0;
    g_state.cam_zoom = 1.0;
    g_state.population = 0;
    g_state.pop_capacity = 0;
    g_state.money = 1000;
    g_state.day = 1;
    g_state.current_tool = TOOL_HOUSE;

    g_state.selected_item_id[0] = '\0';
    g_state.use_item_target = false;
    g_state.build_target = TILE_HOUSE;
    g_state.current_cost = 100;

    // 清空旧 grid 兼容层（新逻辑以 chunk 为准）
    memset(g_state.grid, 0, sizeof(g_state.grid));
    memset(g_state.grid_id, 0, sizeof(g_state.grid_id));
}

void game_set_tool(BuildTool tool) {
    g_state.current_tool = tool;
    g_state.selected_item_id[0] = '\0';
    g_state.use_item_target = false;
    switch (tool) {
        case TOOL_HOUSE:    g_state.current_cost = 100; break;
        case TOOL_ROAD:     g_state.current_cost = 10;  break;
        case TOOL_BULLDOZE: g_state.current_cost = 0;   break;
    }
}

void game_set_item(const char *item_id, const char *tile_name, int cost) {
    snprintf(g_state.selected_item_id, sizeof(g_state.selected_item_id), "%s",
             item_id ? item_id : "");

    TileType t = game_tile_from_name(tile_name);
    g_state.use_item_target = true;
    g_state.build_target = t;
    g_state.current_cost = cost;

    if (t == TILE_HOUSE)      g_state.current_tool = TOOL_HOUSE;
    else if (t == TILE_ROAD)  g_state.current_tool = TOOL_ROAD;
    else                      g_state.current_tool = TOOL_BULLDOZE;
}

void game_clear_item(void) {
    g_state.selected_item_id[0] = '\0';
    g_state.use_item_target = false;
}

static TileType tool_to_tile(void) {
    switch (g_state.current_tool) {
        case TOOL_HOUSE:    return TILE_HOUSE;
        case TOOL_ROAD:     return TILE_ROAD;
        case TOOL_BULLDOZE: return TILE_GRASS;
    }
    return TILE_GRASS;
}

// 世界坐标 -> 当前 tile（走 chunk）
static int world_tile(int wx, int wy) {
    return chunk_get_tile(wx, wy);
}

void game_handle_click(int mx, int my, int button) {
    // 无限地图：不再限制 MAP_W/MAP_H，坐标合法即处理
    // 但保留一个合理边界，避免误点太远导致 chunk 暴涨（可调）
    const int WORLD_LIMIT = 100000;
    if (mx < -WORLD_LIMIT || mx > WORLD_LIMIT ||
        my < -WORLD_LIMIT || my > WORLD_LIMIT) return;

    // 右键拆除
    if (button == 3) {
        int cur = world_tile(mx, my);
        if (cur != TILE_GRASS) {
            chunk_clear_overlay(mx, my);
            g_state.money += 10;
        }
        return;
    }

    if (button != 1) return;

    int current = world_tile(mx, my);
    TileType target = g_state.use_item_target ? g_state.build_target : tool_to_tile();

    if (target == TILE_GRASS) {
        if (current != TILE_GRASS) {
            chunk_clear_overlay(mx, my);
        }
        return;
    }

    // 建造：只能建在草地上
    if (current != TILE_GRASS) return;
    int cost = g_state.current_cost;
    if (g_state.money < cost) return;

    g_state.money -= cost;

    // 存入具体物品 ID
    char id[32] = {0};
    if (g_state.use_item_target && g_state.selected_item_id[0] != '\0') {
        snprintf(id, sizeof(id), "%s", g_state.selected_item_id);
    } else {
        if (target == TILE_HOUSE) snprintf(id, sizeof(id), "house");
        else if (target == TILE_ROAD) snprintf(id, sizeof(id), "road");
    }

    chunk_set_overlay(mx, my, (int)target, id);
}

void game_tick(void) {
    // 注意：无限地图下不能全图遍历统计房屋。
    // 这里改为遍历已加载 chunk 统计，同时 overlay 里的 house 也计入。
    // chunk_foreach 已提供，但为了简单，我们用一个计数近似：
    // 实际项目中建议维护 house_count 增量，这里保持可读性优先。
    int houses = 0;

    // 统计已加载 chunk 中的 house（含 overlay 覆盖后的 base_tile）
    extern void chunk_foreach(void (*fn)(Chunk*, void*), void *user);
    // 由于 C 不允许嵌套函数，用一个静态计数 + 回调在下面实现
    // 这里为了不引入新文件，直接用 chunk_count 近似不可行，
    // 所以改用 chunk 暴露的遍历回调（见 chunk.h 已声明 chunk_foreach）
    // 为避免重复定义，game.c 里通过一个 static 函数 + 全局临时计数实现。
    // 简化：直接调用 chunk_count 无法得到 house 数，故改用下面的遍历。
    // （若你不想加回调，可维护 g_state.house_count，在建造/拆除时增减）
    // 这里给出回调版本：
    // 见文件末尾 game_count_houses()
    houses = game_count_houses();

    g_state.pop_capacity = houses * 10;

    if (g_state.population < g_state.pop_capacity) {
        g_state.population += (g_state.pop_capacity - g_state.population) / 5 + 1;
        if (g_state.population > g_state.pop_capacity)
            g_state.population = g_state.pop_capacity;
    } else if (g_state.population > g_state.pop_capacity) {
        g_state.population -= (g_state.population - g_state.pop_capacity) / 2 + 1;
        if (g_state.population < g_state.pop_capacity)
            g_state.population = g_state.pop_capacity;
    }

    g_state.money += g_state.population / 2;
    g_state.day++;
}

// 统计所有已加载 chunk 中 TILE_HOUSE 数量
static int g_house_tmp = 0;
static void count_house_visitor(Chunk *c, void *user) {
    (void)user;
    for (int ly = 0; ly < CHUNK_SIZE; ly++) {
        for (int lx = 0; lx < CHUNK_SIZE; lx++) {
            if (c->base_tile[ly][lx] == TILE_HOUSE) g_house_tmp++;
        }
    }
}

int game_count_houses(void) {
    g_house_tmp = 0;
    chunk_foreach(count_house_visitor, NULL);
    return g_house_tmp;
}

bool game_is_road(int x, int y) {
    return chunk_get_tile(x, y) == TILE_ROAD;
}

// ---------- 存档挂钩 ----------
void game_save_to(const char *path) {
    SaveData d;
    memset(&d, 0, sizeof(d));
    d.version = SAVE_VERSION;
    d.world_seed = g_world_seed;
    d.cam_offset_x = g_state.cam_offset_x;
    d.cam_offset_y = g_state.cam_offset_y;
    d.cam_zoom = g_state.cam_zoom;
    d.money = g_state.money;
    d.population = g_state.population;
    d.pop_capacity = g_state.pop_capacity;
    d.day = g_state.day;

    save_collect_overlay(&d);
    bool ok = save_game(path, &d);
    save_data_free(&d);
    if (ok) fprintf(stderr, "[game] 已保存到 %s\n", path);
    else    fprintf(stderr, "[game] 保存失败: %s\n", path);
}

bool game_load_from(const char *path) {
    SaveData d;
    if (!load_game(path, &d)) {
        fprintf(stderr, "[game] 读档失败: %s\n", path);
        return false;
    }

    g_world_seed = d.world_seed;
    chunk_world_init(g_world_seed);
    save_apply_overlay(&d);

    g_state.cam_offset_x = d.cam_offset_x;
    g_state.cam_offset_y = d.cam_offset_y;
    g_state.cam_zoom = d.cam_zoom;
    g_state.money = d.money;
    g_state.population = d.population;
    g_state.pop_capacity = d.pop_capacity;
    g_state.day = d.day;

    save_data_free(&d);
    fprintf(stderr, "[game] 读档成功: %s\n", path);
    return true;
}